#include "components/configuration_export/import_export.h"

#include "base/csv_reader.h"
#include "base/csv_writer.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "common/format.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/task_manager.h"

#include <algorithm>
#include <set>

namespace {

base::string16 FormatReferenceCell(const base::string16& title,
                                   const scada::NodeId& prop_type_id) {
  return base::StringPrintf(
      L"%ls @%ls", title.c_str(),
      base::SysNativeMBToWide(NodeIdToScadaString(prop_type_id)).c_str());
}

scada::NodeId ParseReferenceCell(base::StringPiece16 s) {
  auto p = s.rfind(L'@');
  if (p == base::StringPiece::npos)
    return scada::NodeId();
  auto n = s.substr(p + 1);
  return NodeIdFromScadaString(base::SysWideToNativeMB(n.as_string()));
}

void ScanDeleteNodes(const NodeRef& parent_node,
                     const scada::NodeId& type_id,
                     const std::set<scada::NodeId>& exclude_ids,
                     std::vector<scada::NodeId>& results) {
  for (const auto& node : parent_node.targets(scada::id::Organizes)) {
    if (IsInstanceOf(node, type_id)) {
      if (exclude_ids.find(node.node_id()) == exclude_ids.end())
        results.emplace_back(node.node_id());
    }
    ScanDeleteNodes(node, type_id, exclude_ids, results);
  }
}

scada::NodeId GetBuiltInDataTypeId(const NodeRef& data_type) {
  for (int i = scada::Variant::EMPTY + 1; i < scada::Variant::COUNT; ++i) {
    auto built_in_data_type_id =
        scada::ToNodeId(static_cast<scada::Variant::Type>(i));
    assert(!built_in_data_type_id.is_null());
    if (IsSubtypeOf(data_type, built_in_data_type_id))
      return built_in_data_type_id;
  }
  return data_type.node_id();
}

}  // namespace

ExportData ReadExportData(NodeService& node_service, CsvReader& reader) {
  std::wstring cell;

  if (!reader.NextRow())
    throw ResourceError{L"Нет строки заголовка"};

  // Skip Id, Parent, Type, Name.
  for (int i = 0; i < 4; ++i) {
    if (!reader.NextCell(cell))
      throw ResourceError{L"Неверный формат имени столбца"};
  }

  std::vector<ExportData::Property> props;
  while (reader.NextCell(cell)) {
    auto prop_type_id = ParseReferenceCell(cell);
    if (prop_type_id.is_null())
      throw ResourceError{L"Неверный формат имени столбца"};
    auto prop_decl = node_service.GetNode(prop_type_id);
    bool reference = prop_decl.node_class() == scada::NodeClass::ReferenceType;
    // TODO: Read display name.
    props.push_back({std::move(prop_type_id), {}, reference});
  }

  std::vector<ExportData::Node> nodes;
  while (reader.NextRow()) {
    // Id.
    if (!reader.NextCell(cell))
      throw ResourceError{L"Ошибка при чтении идентификатора"};
    auto node_id = NodeIdFromScadaString(base::SysWideToNativeMB(cell));

    // Parent.
    if (!reader.NextCell(cell))
      throw ResourceError{L"Ошибка при чтении родителя"};
    auto parent_id = NodeIdFromScadaString(base::SysWideToNativeMB(cell));
    if (parent_id.is_null())
      throw ResourceError{
          base::StringPrintf(L"Группа '%ls' не найдена", cell.c_str())};

    // Type.
    if (!reader.NextCell(cell))
      throw ResourceError{L"Ошибка при чтении типа"};
    auto type_definition = node_service.GetNode(ParseReferenceCell(cell));
    if (!type_definition)
      throw ResourceError{
          base::StringPrintf(L"Тип с именем '%ls' не найден", cell.c_str())};

    scada::NodeAttributes attrs;
    if (!reader.NextCell(cell))
      throw ResourceError{L"Ошибка при чтении имени"};
    scada::LocalizedText display_name = std::move(cell);

    // Props & refs.
    std::vector<ExportData::PropertyValue> prop_values;
    for (const auto& prop : props) {
      if (!reader.NextCell(cell))
        throw ResourceError(L"Отличающееся число ячеек в строке");

      if (prop.reference) {
        if (type_definition.target(prop.node_id)) {
          auto target_id = ParseReferenceCell(cell);
          // TODO: Read display name.
          prop_values.push_back(
              {prop.node_id, {}, std::move(target_id), {}, true});
        }

      } else {
        if (auto property_declaration = type_definition[prop.node_id]) {
          scada::Variant new_value;
          auto built_in_data_type_id =
              GetBuiltInDataTypeId(property_declaration.data_type());
          if (!StringToValue(cell, built_in_data_type_id, new_value)) {
            auto data_type = node_service.GetNode(built_in_data_type_id);
            auto data_type_name = data_type
                                      ? ToString16(data_type.display_name())
                                      : L"(Неизвестный)";
            throw ResourceError{
                base::StringPrintf(L"Невозможно распознать '%ls' как '%ls'",
                                   cell.c_str(), data_type_name.c_str())};
          }

          prop_values.push_back({prop.node_id, std::move(new_value)});
        }
      }
    }

    // TODO: Read display name.
    nodes.push_back({
        std::move(node_id),
        std::move(parent_id),
        {},
        type_definition.node_id(),
        std::move(display_name),
        std::move(prop_values),
    });
  }

  return {std::move(props), std::move(nodes)};
}

ImportData ImportConfiguration(NodeService& node_service, CsvReader& reader) {
  auto export_data = ReadExportData(node_service, reader);

  ImportData import_data;

  std::set<scada::NodeId> listed_nodes;

  for (const auto& export_node : export_data.nodes) {
    auto node = node_service.GetNode(export_node.node_id);
    if (node)
      listed_nodes.emplace(node.node_id());

    auto type_definition = node_service.GetNode(export_node.type_id);
    if (!type_definition) {
      throw ResourceError{base::StringPrintf(
          L"Тип %ls не найден",
          base::SysNativeMBToWide(NodeIdToScadaString(export_node.type_id))
              .c_str())};
    }

    scada::NodeAttributes attrs;
    if (!node || node.display_name() != export_node.display_name)
      attrs.set_display_name(std::move(export_node.display_name));

    // Props & refs.
    scada::NodeProperties props;
    std::vector<ImportData::Reference> refs;

    for (const auto& export_value : export_node.property_values) {
      if (export_value.reference) {
        auto old_target_id = node.target(export_value.node_id).node_id();
        if (old_target_id != export_value.target_id) {
          refs.push_back(
              {export_value.node_id, old_target_id, export_value.target_id});
        }
      } else {
        if (node) {
          auto value = node[export_value.node_id].value();
          if (value == export_value.value)
            continue;
        }

        props.emplace_back(export_value.node_id, export_value.value);
      }
    }

    if (node) {
      if (!attrs.empty() || !props.empty() || !refs.empty())
        import_data.modify_nodes.push_back(
            {export_node.node_id, type_definition.node_id(),
             export_node.parent_id, std::move(attrs), std::move(props),
             std::move(refs)});
    } else {
      import_data.create_nodes.push_back(
          {export_node.node_id, type_definition.node_id(),
           export_node.parent_id, std::move(attrs), std::move(props),
           std::move(refs)});
    }
  }

  ScanDeleteNodes(node_service.GetNode(data_items::id::DataItems),
                  data_items::id::DataItemType, listed_nodes,
                  import_data.delete_nodes);

  return import_data;
}

void ExportConfiguration(const ExportData& data, CsvWriter& writer) {
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
            : base::string16{});
    writer.WriteCell(node.display_name);
    assert(node.property_values.size() == data.props.size());
    for (const auto& prop_value : node.property_values) {
      if (prop_value.reference) {
        writer.WriteCell(
            !prop_value.target_id.is_null()
                ? FormatReferenceCell(prop_value.target_display_name,
                                      prop_value.target_id)
                : base::string16{});
      } else {
        auto str = prop_value.value.get_or(std::string());
        writer.WriteCell(base::SysNativeMBToWide(str));
      }
    }
  }
}
