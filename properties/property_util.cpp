#include "properties/property_util.h"

#include "base/range_util.h"
#include "common/format.h"
#include "node_service/node_promises.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "properties/property_context.h"
#include "services/task_manager.h"

#include <boost/algorithm/string/predicate.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace {

promise<void> FetchNodeNamesRecursive(
    const NodeRef& parent_node,
    const scada::NodeId& type_definition_id,
    const aui::EditData::AsyncChoiceCallback& callback) {
  return FetchChildren(parent_node).then([=] {
    auto children = parent_node.targets(scada::id::Organizes);

    auto child_names =
        children |
        boost::adaptors::filtered([type_definition_id](const NodeRef& node) {
          return IsInstanceOf(node, type_definition_id);
        }) |
        boost::adaptors::transformed(&GetFullDisplayName) | to_vector;
    callback(child_names, false);

    auto recursive_promises =
        children | boost::adaptors::transformed([type_definition_id, callback](
                                                    const NodeRef& node) {
          return FetchNodeNamesRecursive(node, type_definition_id, callback);
        });

    return make_all_promise_void(recursive_promises);
  });
}

}  // namespace

const std::u16string_view kChoiceNone = u"<Нет>";

void SetTextHelper(const PropertyContext& context,
                   const NodeRef& node,
                   const scada::NodeId& prop_decl_id,
                   std::u16string_view text) {
  const auto& property = node[prop_decl_id];
  if (!property)
    return;

  scada::Variant value;
  if (!StringToValue(text, ToBuiltInDataType(property.data_type().node_id()),
                     value)) {
    return;
  }

  context.task_manager_.PostUpdateTask(node.node_id(), {},
                                       {{prop_decl_id, std::move(value)}});
}

NodeRef FindNodeByNameAndType(const NodeRef& parent_node,
                              const std::u16string_view& name,
                              const scada::NodeId& node_type_id) {
  for (const auto& node : parent_node.targets(scada::id::Organizes)) {
    if (IsInstanceOf(node, node_type_id)) {
      const auto& node_name = GetFullDisplayName(node);
      if (boost::algorithm::iequals(node_name, name))
        return node;
    }
    if (auto n = FindNodeByNameAndType(node, name, node_type_id))
      return n;
  }
  return nullptr;
}

aui::EditData::AsyncChoiceHandler MakeAsyncChoiceHandler(
    const NodeRef& parent,
    const scada::NodeId& type_definition_id) {
  return [parent, type_definition_id](
             const aui::EditData::AsyncChoiceCallback& callback) {
    callback({std::u16string{kChoiceNone}}, false);
    FetchNodeNamesRecursive(parent, type_definition_id, callback)
        .then([callback] { callback({}, true); });
  };
}
