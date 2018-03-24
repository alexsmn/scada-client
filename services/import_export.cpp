#include "import_export.h"

#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/table_reader.h"
#include "base/table_writer.h"
#include "common/format.h"
#include "common/node_id_util.h"
#include "common/node_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "services/task_manager.h"

#include <algorithm>
#include <set>

namespace {

typedef std::vector<NodeRef> Nodes;

void GetNodesRecursive(NodeRef parent_node, Nodes& nodes) {
  for (auto node : parent_node.organizes()) {
    nodes.emplace_back(node);
    GetNodesRecursive(node, nodes);
  }
}

typedef std::vector<NodeRef> Pids;

void GetTypePids(NodeRef type, Pids& pids, bool recursive) {
  if (recursive) {
    if (auto supertype = type.supertype())
      GetTypePids(supertype, pids, true);
  }

  for (auto p : type.properties())
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

void ScanDeleteNodes(NodeRef parent_node,
                     const scada::NodeId& type_id,
                     const std::set<scada::NodeId>& exclude_ids,
                     std::vector<scada::NodeId>& results) {
  for (auto node : parent_node.organizes()) {
    if (IsInstanceOf(node, type_id)) {
      if (exclude_ids.find(node.id()) == exclude_ids.end())
        results.emplace_back(node.id());
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
      listed_nodes.emplace(node.id());

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
    auto type = node_service.GetNode(ParseReferenceCell(cell));
    if (!type)
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
      if (auto prop = type[pid]) {
        scada::Variant new_value;
        if (!StringToValue(cell.c_str(), prop.data_type().id(), new_value)) {
          auto prop_type = node_service.GetNode(prop.data_type().id());
          auto prop_type_name = prop_type ? ToString16(prop_type.display_name())
                                          : L"(Неизвестный)";
          throw ResourceError{base::StringPrintf(
              L"Невозможно распознать значение ячейки '%ls' с типом '%ls'",
              cell.c_str(), prop_type_name.c_str())};
        }

        if (node) {
          auto value = node[prop.id()].value();
          if (value == new_value)
            continue;
        }

        props.emplace_back(prop.id(), std::move(new_value));

      } else if (auto target = type.target(pid)) {
        auto referenced_id = ParseReferenceCell(cell);
        auto old_referenced_id = target.id();
        if (old_referenced_id != referenced_id)
          refs.push_back({pid, old_referenced_id, referenced_id});
      }
    }

    if (node) {
      if (!attrs.empty() || !props.empty() || !refs.empty())
        import_data.modify_nodes.push_back({node_id, type.id(), parent_id,
                                            std::move(attrs), std::move(props),
                                            std::move(refs)});
    } else {
      import_data.create_nodes.push_back({node_id, type.id(), parent_id,
                                          std::move(attrs), std::move(props),
                                          std::move(refs)});
    }
  }

  ScanDeleteNodes(node_service.GetNode(id::DataItems), id::DataItemType,
                  listed_nodes, import_data.delete_nodes);

  return import_data;
}

void ExportConfiguration(NodeService& node_service, TableWriter& writer) {
  Pids props;
  GetTypePids(node_service.GetNode(id::DiscreteItemType), props, true);
  GetTypePids(node_service.GetNode(id::AnalogItemType), props, false);

  Nodes nodes;
  GetNodesRecursive(node_service.GetNode(id::DataItems), nodes);

  // Headers
  writer.StartRow();
  writer.WriteCell(kNodeIdTitle);
  writer.WriteCell(L"Родитель");
  writer.WriteCell(L"Тип");
  writer.WriteCell(L"Имя");
  for (auto prop : props)
    writer.WriteCell(FormatReferenceCell(prop.display_name(), prop.id()));

  // Rows
  for (auto node : nodes) {
    assert(node);
    writer.StartRow();
    writer.WriteCell(base::SysNativeMBToWide(NodeIdToScadaString(node.id())));
    writer.WriteCell(
        base::SysNativeMBToWide(NodeIdToScadaString(node.parent().id())));
    auto type = node.type_definition();
    writer.WriteCell(type ? FormatReferenceCell(type.display_name(), type.id())
                          : base::string16{});
    writer.WriteCell(node.display_name());
    for (auto prop : props) {
      if (prop.node_class() == scada::NodeClass::ReferenceType) {
        auto referenced_node = node.target(prop.id());
        writer.WriteCell(referenced_node ? FormatReferenceCell(
                                               referenced_node.display_name(),
                                               referenced_node.id())
                                         : base::string16{});
      } else {
        auto value = node[prop.id()].value();
        auto str = value.get_or(std::string());
        writer.WriteCell(base::SysNativeMBToWide(str));
      }
    }
  }
}
