#include "export/configuration/export_data_writer.h"

#include "base/csv_writer.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "export/configuration/export_data.h"
#include "model/node_id_util.h"

namespace {

std::u16string FormatReferenceCell(const std::u16string& title,
                                   const scada::NodeId& prop_decl_id) {
  return base::StringPrintf(
      u"%ls @%ls", title.c_str(),
      base::UTF8ToUTF16(NodeIdToScadaString(prop_decl_id)).c_str());
}

}  // namespace

void WriteExportData(const ExportData& data, CsvWriter& writer) {
  // Headers
  writer.StartRow();
  writer.WriteCell(kNodeIdTitle);
  writer.WriteCell(u"Родитель");
  writer.WriteCell(u"Тип");
  writer.WriteCell(u"Имя");
  for (const auto& prop : data.props)
    writer.WriteCell(FormatReferenceCell(prop.display_name, prop.prop_decl_id));

  // Rows
  for (const auto& node : data.nodes) {
    writer.StartRow();
    writer.WriteCell(NodeIdToScadaString(node.node_id));
    writer.WriteCell(NodeIdToScadaString(node.parent_id));
    writer.WriteCell(
        !node.type_id.is_null()
            ? FormatReferenceCell(node.type_display_name, node.type_id)
            : std::u16string{});
    writer.WriteCell(node.display_name);

    for (const ExportData::Property& prop : data.props) {
      auto i = std::ranges::find(node.property_values, prop.prop_decl_id,
                                 &ExportData::PropertyValue::prop_decl_id);
      if (i == node.property_values.end()) {
        writer.SkipCell();
        continue;
      }

      const ExportData::PropertyValue& prop_value = *i;

      if (prop_value.reference) {
        assert(!prop_value.target_id.is_null());
        writer.WriteCell(FormatReferenceCell(prop_value.target_display_name,
                                             prop_value.target_id));
      } else {
        assert(!prop_value.value.is_null());
        auto str = prop_value.value.get_or(std::string());
        writer.WriteCell(str);
      }
    }
  }
}
