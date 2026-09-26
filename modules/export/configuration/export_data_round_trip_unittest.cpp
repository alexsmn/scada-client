#include "export/configuration/export_data.h"
#include "export/configuration/export_data_reader.h"
#include "export/configuration/export_data_writer.h"

#include "address_space/test/test_scada_node_states.h"
#include "base/csv_reader.h"
#include "base/csv_writer.h"
#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "node_service/static/static_node_service.h"

#include <gmock/gmock.h>
#include <sstream>
#include <string>

using namespace testing;

namespace {

// The configuration export file is the CSV that `WriteExportData` produces and
// `ExportDataReader` parses back; the Qt client's "Export/Import Configuration
// to Excel" commands are those two ends joined by a file the operator edits in
// between (backlog 34). Nothing pinned that the two ends agree, so every
// case here writes an `ExportData`, reads it back, and compares.
class ExportDataRoundTripTest : public Test {
 protected:
  void SetUp() override { node_service_.AddAll(GetScadaNodeStates()); }

  std::string Write(const ExportData& data) const {
    std::ostringstream stream;
    CsvWriter writer{stream};
    WriteExportData(data, writer);
    return stream.str();
  }

  // Reads the way `ImportConfigurationCommand::LoadExportData` does, with the
  // `Id` signature that lets the reader pick up a re-saved file's separator.
  ExportData Read(const std::string& csv) {
    std::istringstream stream{csv};
    CsvReader reader{stream, kNodeIdTitle};
    return ExportDataReader{node_service_, reader}.Read();
  }

  StaticNodeService node_service_;
};

// The reader does not carry the display names the writer puts beside each id:
// the id after `@` is the identity, and the text before it is only there for
// the person editing the file. Clears them so a comparison is about what an
// import actually consumes.
ExportData WithoutDisplayNames(ExportData data) {
  for (ExportData::Property& prop : data.props)
    prop.display_name = {};
  for (ExportData::Node& node : data.nodes) {
    node.type_display_name = {};
    for (ExportData::PropertyValue& value : node.property_values)
      value.target_display_name = {};
  }
  return data;
}

ExportData MakeSampleExportData() {
  namespace id = scada::data_items::id;
  const scada::NodeId group_id{1, scada::NamespaceIndexes::GROUP};

  return ExportData{
      .props =
          {
              {.prop_decl_id = id::DataItemType_Alias,
               .display_name = u"Alias"},
              {.prop_decl_id = id::DataItemType_Simulated,
               .display_name = u"Simulated"},
              {.prop_decl_id = id::DataItemType_Severity,
               .display_name = u"Severity"},
              {.prop_decl_id = id::DataItemType_StalePeriod,
               .display_name = u"Stale period, s"},
              {.prop_decl_id = id::DiscreteItemType_Inversion,
               .display_name = u"Inversion"},
              {.prop_decl_id = id::HasTsFormat,
               .display_name = u"Parameters",
               .reference = true},
              {.prop_decl_id = id::AnalogItemType_DisplayFormat,
               .display_name = u"Format"},
              {.prop_decl_id = id::AnalogItemType_IrHi,
               .display_name = u"Physical maximum"},
          },
      .nodes = {
          {.node_id = {22, scada::NamespaceIndexes::TS},
           .parent_id = group_id,
           .type_display_name = u"Discrete item",
           .type_id = id::DiscreteItemType,
           // A comma and quotes: both must survive CSV quoting.
           .display_name = u"Block, \"Local\"",
           .property_values =
               {
                   {.prop_decl_id = id::DataItemType_Alias,
                    .value = std::string{"ts106"}},
                   {.prop_decl_id = id::DataItemType_Simulated, .value = false},
                   {.prop_decl_id = id::DataItemType_Severity,
                    .value = scada::Int32{90}},
                   {.prop_decl_id = id::DataItemType_StalePeriod,
                    .value = scada::Int32{0}},
                   {.prop_decl_id = id::DiscreteItemType_Inversion,
                    .value = true},
                   {.prop_decl_id = id::HasTsFormat,
                    .target_id = {2, scada::NamespaceIndexes::TS_FORMAT},
                    .target_display_name = u"No/Yes",
                    .reference = true},
               }},
          {.node_id = {76, scada::NamespaceIndexes::TIT},
           .parent_id = group_id,
           .type_display_name = u"Analog item",
           .type_id = id::AnalogItemType,
           .display_name = u"Q",
           .property_values =
               {
                   {.prop_decl_id = id::DataItemType_Alias,
                    .value = std::string{"tit1143"}},
                   {.prop_decl_id = id::DataItemType_Severity,
                    .value = scada::Int32{1}},
                   {.prop_decl_id = id::AnalogItemType_DisplayFormat,
                    .value = std::string{"0.#"}},
                   {.prop_decl_id = id::AnalogItemType_IrHi, .value = 2000.5},
               }},
      }};
}

TEST_F(ExportDataRoundTripTest, WrittenFileReadsBackUnchanged) {
  const ExportData data = MakeSampleExportData();

  EXPECT_EQ(Read(Write(data)), WithoutDisplayNames(data));
}

// Excel in a locale whose list separator is `;` re-saves the file with it.
// The reader keys on the leading `Id` cell to learn the separator, so an
// edited-and-resaved file must import like the original.
TEST_F(ExportDataRoundTripTest, SemicolonResaveReadsBackUnchanged) {
  const ExportData data = MakeSampleExportData();

  std::ostringstream stream;
  CsvWriter writer{stream};
  writer.delimiter = ';';
  WriteExportData(data, writer);

  EXPECT_EQ(Read(stream.str()), WithoutDisplayNames(data));
}

// A display name edited in Excel with Alt+Enter holds a line break. The writer
// quotes it across lines, as RFC 4180 allows, and the reader used to split the
// record there and reject the file.
TEST_F(ExportDataRoundTripTest, MultiLineCellReadsBackUnchanged) {
  ExportData data = MakeSampleExportData();
  data.nodes[0].display_name = u"Two\nlines";

  EXPECT_EQ(Read(Write(data)), WithoutDisplayNames(data));
}

// The header's last column needs quoting when its title holds a comma. A
// quoted last cell used to be followed by a phantom empty one, which the reader
// took for a property column and rejected.
TEST_F(ExportDataRoundTripTest, QuotedLastHeaderReadsBackUnchanged) {
  ExportData data = MakeSampleExportData();
  data.props.back().display_name = u"Physical maximum, units";

  EXPECT_EQ(Read(Write(data)), WithoutDisplayNames(data));
}

// An empty cell means "no value", not "empty string": the reader drops it, so
// a property the node does not carry stays absent after the round trip rather
// than turning into a default.
TEST_F(ExportDataRoundTripTest, AbsentPropertyStaysAbsent) {
  const ExportData data = MakeSampleExportData();

  const ExportData read = Read(Write(data));

  ASSERT_EQ(read.nodes.size(), 2u);
  EXPECT_EQ(read.nodes[1].FindPropValue(
                scada::data_items::id::DiscreteItemType_Inversion),
            nullptr);
}

}  // namespace
