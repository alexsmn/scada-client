#include "components/device_metrics/device_metrics_command.h"

#include "base/range_util.h"
#include "client_utils.h"
#include "common/formula_util.h"
#include "common_resources.h"
#include "components/sheet/sheet_component.h"
#include "controls/color.h"
#include "model/devices_node_ids.h"
#include "model/scada_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "window_info.h"

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>

namespace {

promise<NodeRef> FetchNode(const NodeRef& node,
                           const NodeFetchStatus& requested_status) {
  auto promise = make_promise<NodeRef>();
  node.Fetch(requested_status,
             [promise](const NodeRef& node) mutable { promise.resolve(node); });
  return promise;
}

promise<std::vector<NodeRef>> CollectChildren(
    const NodeRef& parent_node,
    const scada::NodeId& type_definition_id) {
  return FetchNode(parent_node, NodeFetchStatus::ChildrenOnly())
      .then([](const NodeRef& fetched_node) {
        return make_all_promise(
            fetched_node.targets(scada::id::Organizes) |
            boost::adaptors::transformed([](const NodeRef& node) {
              return FetchNode(node, NodeFetchStatus::NodeOnly());
            }));
      })
      .then([type_definition_id](const std::vector<NodeRef>& fetched_children) {
        return fetched_children |
               boost::adaptors::filtered(
                   [type_definition_id](const NodeRef& node) {
                     return IsInstanceOf(node, type_definition_id);
                   }) |
               to_vector;
      });
}

std::vector<NodeRef> JoinAll(
    const NodeRef& parent_node,
    base::span<const std::vector<NodeRef>> recursive_children) {
  std::vector<NodeRef> result;
  result.push_back(parent_node);
  for (auto& part : recursive_children) {
    result.insert(result.end(), std::make_move_iterator(part.begin()),
                  std::make_move_iterator(part.end()));
  }
  return result;
}

promise<std::vector<NodeRef>> CollectNodesRecursive(
    const NodeRef& parent_node,
    const scada::NodeId& type_definition_id) {
  return CollectChildren(parent_node, type_definition_id)
      .then([type_definition_id](const std::vector<NodeRef>& children) {
        return make_all_promise(
            children | boost::adaptors::transformed([type_definition_id](
                                                        const NodeRef& node) {
              return CollectNodesRecursive(node, type_definition_id);
            }));
      })
      .then([parent_node](
                const std::vector<std::vector<NodeRef>>& recursive_children) {
        return JoinAll(parent_node, recursive_children);
      });
}

std::set<NodeRef> CollectTypeDefinitions(base::span<const NodeRef> devices) {
  return devices |
         boost::adaptors::transformed(std::mem_fn(&NodeRef::type_definition)) |
         to_set;
}

std::vector<NodeRef> GetSupertypes(NodeRef type_definition) {
  std::vector<NodeRef> supertypes;
  for (; type_definition; type_definition = type_definition.supertype())
    supertypes.push_back(type_definition);
  return supertypes;
}

auto GetDataVariableDecls(const NodeRef& type_defintion) {
  return GetSupertypes(type_defintion) |
         boost::adaptors::transformed(&GetDataVariables) | flattened;
}

std::set<NodeRef> CollectVariables(base::span<const NodeRef> devices) {
  return CollectTypeDefinitions(devices) |
         boost::adaptors::transformed(&GetDataVariableDecls) | flattened |
         to_set;
}

WindowDefinition MakeDeviceMetricsWindowDefinitionSync(
    std::wstring title,
    base::span<const NodeRef> devices) {
  WindowDefinition win(kSheetWindowInfo);
  win.title = std::move(title);

  // Header column.
  {
    WindowItem& cell = win.AddItem("Column");
    cell.SetInt("ix", 1);
    cell.SetInt("width", 200);
  }

  const aui::Color kHeaderColor = aui::Rgba{227, 227, 227};

  // Header.
  auto data_variable_decls = CollectVariables(devices) | to_vector;
  for (size_t i = 0; i < data_variable_decls.size(); ++i) {
    WindowItem& cell = win.AddItem("SheetCell");
    cell.SetInt("row", i + 2);
    cell.SetInt("col", 1);
    cell.SetString("text", ToString16(data_variable_decls[i].display_name()));
    cell.SetString("color", aui::ColorToString(kHeaderColor));
    cell.SetString("align", "right");
  }

  // Items.
  for (size_t i = 0; i < devices.size(); ++i) {
    const auto& device = devices[i];

    // Item header.
    {
      WindowItem& cell = win.AddItem("SheetCell");
      cell.SetInt("row", 1);
      cell.SetInt("col", i + 2);
      cell.SetString("text", ToString16(device.display_name()));
      cell.SetString("color", aui::ColorToString(kHeaderColor));
    }

    // Metric cells.
    for (size_t j = 0; j < data_variable_decls.size(); ++j) {
      if (auto data_variable = device[data_variable_decls[j].browse_name()]) {
        auto& cell = win.AddItem("SheetCell");
        cell.SetInt("row", j + 2);
        cell.SetInt("col", i + 2);
        auto formula = MakeNodeIdFormula(data_variable.node_id());
        cell.SetString("text", '=' + formula);
      }
    }
  }

  return win;
}

}  // namespace

promise<WindowDefinition> MakeDeviceMetricsWindowDefinition(
    const NodeRef& device) {
  if (!device.type_definition()) {
    return make_rejected_promise<WindowDefinition>(
        std::runtime_error{"Device type"});
  }

  return CollectNodesRecursive(device, devices::id::DeviceType)
      .then([device](const std::vector<NodeRef>& devices) {
        auto title = ToString16(device.display_name());
        return MakeDeviceMetricsWindowDefinitionSync(std::move(title), devices);
      });
}
