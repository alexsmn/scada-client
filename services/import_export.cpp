#include "services/import_export.h"

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/table_reader.h"
#include "base/table_writer.h"
#include "common/format.h"
#include "common/node_service.h"
#include "common/node_util.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"
#include "services/task_manager.h"

#include <algorithm>
#include <set>

namespace {

using NodeRefs = std::vector<NodeRef>;

void GetNodesRecursive(NodeRef parent_node, NodeRefs& nodes) {
  for (const auto& node : parent_node.targets(scada::id::Organizes)) {
    nodes.emplace_back(node);
    GetNodesRecursive(node, nodes);
  }
}

void GetTypePids(const NodeRef& type, NodeRefs& pids, bool recursive) {
  if (recursive) {
    if (auto supertype = type.supertype())
      GetTypePids(supertype, pids, true);
  }

  for (auto p : type.targets(scada::id::HasProperty))
    pids.emplace_back(p);
  for (auto r : type.references())
    pids.emplace_back(r.reference_type);
}

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

}  // namespace

ImportData ImportConfiguration(NodeService& node_service, TableReader& reader) {
  ImportData import_data;

  std::set<scada::NodeId> listed_nodes;

  std::wstring cell;

  if (!reader.NextRow())
    throw ResourceError{L"Нет строки заголовка"};

  // Skip Id, Parent, Type, Name.
  for (int i = 0; i < 4; ++i) {
    if (!reader.NextCell(cell))
      throw ResourceError{L"Неверный формат имени столбца"};
  }

  std::vector<scada::NodeId> prop_type_ids;
  while (reader.NextCell(cell)) {
    auto prop_type_id = ParseReferenceCell(cell);
    if (prop_type_id.is_null())
      throw ResourceError{L"Неверный формат имени столбца"};
    prop_type_ids.emplace_back(std::move(prop_type_id));
  }

  while (reader.NextRow()) {
    // Id.
    if (!reader.NextCell(cell))
      throw ResourceError{L"Ошибка при чтении идентификатора"};
    const auto node_id = NodeIdFromScadaString(base::SysWideToNativeMB(cell));

    auto node = node_service.GetNode(node_id);
    if (node)
      listed_nodes.emplace(node.node_id());

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
          base::StringPrintf(L"Тип с именем '%l' не найден", cell.c_str())};

    scada::NodeAttributes attrs;
    if (!reader.NextCell(cell))
      throw ResourceError{L"Ошибка при чтении имени"};
    auto display_name = std::move(cell);
    if (!node || node.display_name() != display_name)
      attrs.set_display_name(std::move(display_name));

    // Props & refs.
    scada::NodeProperties props;
    std::vector<ImportData::Reference> refs;
    size_t pid_index = 0;
    while (reader.NextCell(cell)) {
      auto pid = prop_type_ids[pid_index++];
      if (auto property_declaration = type_definition[pid]) {
        scada::Variant new_value;
        if (!StringToValue(cell, property_declaration.data_type().node_id(),
                           new_value)) {
          auto prop_type = property_declaration.data_type();
          auto prop_type_name = prop_type ? ToString16(prop_type.display_name())
                                          : L"(Неизвестный)";
          throw ResourceError{base::StringPrintf(
              L"Невозможно распознать значение ячейки '%ls' с типом '%ls'",
              cell.c_str(), prop_type_name.c_str())};
        }

        if (node) {
          auto value = node[property_declaration.node_id()].value();
          if (value == new_value)
            continue;
        }

        props.emplace_back(property_declaration.node_id(),
                           std::move(new_value));

      } else if (type_definition.target(pid)) {
        auto referenced_id = ParseReferenceCell(cell);
        auto old_target_id = node.target(pid).node_id();
        if (old_target_id != referenced_id)
          refs.push_back({pid, old_target_id, referenced_id});
      }
    }

    if (node) {
      if (!attrs.empty() || !props.empty() || !refs.empty())
        import_data.modify_nodes.push_back({node_id, type_definition.node_id(),
                                            parent_id, std::move(attrs),
                                            std::move(props), std::move(refs)});
    } else {
      import_data.create_nodes.push_back({node_id, type_definition.node_id(),
                                          parent_id, std::move(attrs),
                                          std::move(props), std::move(refs)});
    }
  }

  ScanDeleteNodes(node_service.GetNode(data_items::id::DataItems),
                  data_items::id::DataItemType, listed_nodes,
                  import_data.delete_nodes);

  return import_data;
}

void ExportConfiguration(NodeService& node_service, TableWriter& writer) {
  NodeRefs props;
  GetTypePids(node_service.GetNode(data_items::id::DiscreteItemType), props,
              true);
  GetTypePids(node_service.GetNode(data_items::id::AnalogItemType), props,
              false);

  NodeRefs nodes;
  GetNodesRecursive(node_service.GetNode(data_items::id::DataItems), nodes);

  // Headers
  writer.StartRow();
  writer.WriteCell(kNodeIdTitle);
  writer.WriteCell(L"Родитель");
  writer.WriteCell(L"Тип");
  writer.WriteCell(L"Имя");
  for (auto prop : props)
    writer.WriteCell(FormatReferenceCell(prop.display_name(), prop.node_id()));

  // Rows
  for (auto node : nodes) {
    assert(node);
    writer.StartRow();
    writer.WriteCell(
        base::SysNativeMBToWide(NodeIdToScadaString(node.node_id())));
    writer.WriteCell(
        base::SysNativeMBToWide(NodeIdToScadaString(node.parent().node_id())));
    auto type = node.type_definition();
    writer.WriteCell(
        type ? FormatReferenceCell(type.display_name(), type.node_id())
             : base::string16{});
    writer.WriteCell(node.display_name());
    for (auto prop : props) {
      if (prop.node_class() == scada::NodeClass::ReferenceType) {
        auto referenced_node = node.target(prop.node_id());
        writer.WriteCell(referenced_node ? FormatReferenceCell(
                                               referenced_node.display_name(),
                                               referenced_node.node_id())
                                         : base::string16{});
      } else {
        auto value = node[prop.node_id()].value();
        auto str = value.get_or(std::string());
        writer.WriteCell(base::SysNativeMBToWide(str));
      }
    }
  }
}
