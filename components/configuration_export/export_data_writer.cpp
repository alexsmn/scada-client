#include "components/configuration_export/export_data_writer.h"

#include "base/csv_writer.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "components/configuration_export/export_data.h"
#include "model/node_id_util.h"

namespace {

std::wstring FormatReferenceCell(const std::wstring& title,
                                 const scada::NodeId& prop_type_id) {
  return base::StringPrintf(
      L"%ls @%ls", title.c_str(),
      base::SysNativeMBToWide(NodeIdToScadaString(prop_type_id)).c_str());
}

}  // namespace

void WriteExportData(const ExportData& data, CsvWriter& writer) {
  // Headers
  writer.StartRow();
  writer.WriteCell(kNodeIdTitle);
  writer.WriteCell(L"Родитель");
  writer.WriteCell(L"Тип");
  writer.WriteCell(L"Имя");
  for (const auto& prop : data.props)
    writer.WriteCell(FormatReferenceCell(prop.display_name, prop.node_id));

  // Rows
  for (const auto& node : data.nodes) {
    writer.StartRow();
    writer.WriteCell(
        base::SysNativeMBToWide(NodeIdToScadaString(node.node_id)));
    writer.WriteCell(
        base::SysNativeMBToWide(NodeIdToScadaString(node.parent_id)));
    writer.WriteCell(
        !node.type_id.is_null()
            ? FormatReferenceCell(node.type_display_name, node.type_id)
            : std::wstring{});
    writer.WriteCell(node.display_name);
    assert(node.property_values.size() == data.props.size());
    for (const auto& prop_value : node.property_values) {
      if (prop_value.reference) {
        writer.WriteCell(
            !prop_value.target_id.is_null()
                ? FormatReferenceCell(prop_value.target_display_name,
                                      prop_value.target_id)
                : std::wstring{});
      } else {
        auto str = prop_value.value.get_or(std::string());
        writer.WriteCell(base::SysNativeMBToWide(str));
      }
    }
  }
}
