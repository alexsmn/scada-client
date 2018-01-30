#include "client_utils.h"

#include "base/color.h"
#include "base/strings/stringprintf.h"
#include "common/browse_util.h"
#include "common/formula_util.h"
#include "common/node_ref.h"
#include "common/node_ref_util.h"
#include "common/node_service.h"
#include "common/scada_node_ids.h"
#include "common_resources.h"
#include "components/main/main_window.h"
#include "translation.h"
#include "window_definition.h"
#include "window_info.h"

struct DeviceMetricsItem {
  NodeRef node;
  int level;
  std::vector<std::string> formulas;
};

static std::string MakeBoolFormula(const std::string& subexpr,
                                   const std::string& true_string,
                                   const std::string& false_string) {
  return base::StringPrintf("IF(%s, \"%s\", \"%s\")", subexpr.c_str(),
                            true_string.c_str(), false_string.c_str());
}

NodeRef FindNodeByName(const std::vector<NodeRef>& nodes,
                       const std::string& browse_name) {
  for (auto& node : nodes) {
    if (node.browse_name().name() == browse_name)
      return node;
  }
  return nullptr;
}

struct DeviceMetricsViewTable {
  std::vector<NodeRef> component_declarations;
  std::vector<DeviceMetricsItem> items;
};

template <class Callback>
struct DeviceMetricsViewBuilder
    : public std::enable_shared_from_this<DeviceMetricsViewBuilder<Callback>> {
  DeviceMetricsViewBuilder(scada::ViewService& view_service,
                           NodeService& node_service,
                           const Callback& callback)
      : view_service_{view_service},
        node_service_{node_service},
        callback_{callback} {}

  ~DeviceMetricsViewBuilder() { callback_(std::move(table_)); }

  void Build(const NodeRef& device) {
    for (auto type_definition = device.type_definition(); type_definition;
         type_definition = type_definition.supertype()) {
      const auto& component_declarations = type_definition.components();
      table_.component_declarations.insert(table_.component_declarations.end(),
                                           component_declarations.begin(),
                                           component_declarations.end());
    }
    AddDeviceRecursive(device, 0);
  }

  void AddDeviceRecursive(const NodeRef& node, int level) {
    assert(IsInstanceOf(node, id::DeviceType));
    AddDevice(node, level);

    auto self = shared_from_this();
    auto nested_level = level + 1;
    BrowseNodes(view_service_, node_service_,
                {node.id(), scada::BrowseDirection::Forward,
                 scada::id::Organizes, true},
                [self, nested_level](const scada::Status& status,
                                     const std::vector<NodeRef>& nodes) {
                  for (auto& node : nodes)
                    self->AddDeviceRecursive(node, nested_level);
                });
  }

  void AddDevice(const NodeRef& device, int level) {
    DeviceMetricsItem item{device, level};
    item.formulas.resize(table_.component_declarations.size());
    for (size_t i = 0; i < table_.component_declarations.size(); ++i) {
      const auto& component_declaration = table_.component_declarations[i];
      if (const auto& component = device[component_declaration.id()])
        item.formulas[i] = MakeNodeIdFormula(component.id());
    }
    table_.items.emplace_back(std::move(item));
  }

  scada::ViewService& view_service_;
  NodeService& node_service_;
  DeviceMetricsViewTable table_;
  const Callback callback_;
};

// Callback = void(DeviceMetricsViewTable)
template <class Callback>
void BuildDeviceMetricsView(scada::ViewService& view_service,
                            NodeService& node_service,
                            const NodeRef& device,
                            const Callback& callback) {
  std::make_shared<DeviceMetricsViewBuilder<Callback>>(view_service,
                                                       node_service, callback)
      ->Build(device);
}

void PrepareDeviceMetricsView(scada::ViewService& view_service,
                              NodeService& node_service,
                              const NodeRef& device,
                              const WindowDefinitionCallback& callback) {
  if (!device.type_definition()) {
    callback(WindowDefinition{});
    return;
  }

  BuildDeviceMetricsView(
      view_service, node_service, device,
      [device, callback](const DeviceMetricsViewTable& table) {
        WindowDefinition win(GetWindowInfo(ID_SHEET_VIEW));
        win.title = ToString16(device.display_name());

        // Header column.
        {
          WindowItem& cell = win.AddItem("Column");
          cell.SetInt("ix", 1);
          cell.SetInt("width", 200);
        }

        const SkColor kHeaderColor = SkColorSetRGB(227, 227, 227);

        // Header.
        for (size_t i = 0; i < table.component_declarations.size(); ++i) {
          WindowItem& cell = win.AddItem("SheetCell");
          cell.SetInt("row", i + 2);
          cell.SetInt("col", 1);
          // TODO: Display name.
          cell.SetString(
              "text",
              ToString16(table.component_declarations[i].display_name()));
          cell.SetString("color", palette::ColorToString(kHeaderColor));
          cell.SetString("align", "right");
        }

        // Items.
        for (size_t i = 0; i < table.items.size(); ++i) {
          auto& item = table.items[i];

          // Item header.
          {
            WindowItem& cell = win.AddItem("SheetCell");
            cell.SetInt("row", 1);
            cell.SetInt("col", i + 2);
            cell.SetString("text", ToString16(item.node.display_name()));
            cell.SetString("color", palette::ColorToString(kHeaderColor));
          }

          // Metric cells.
          for (size_t j = 0; j < table.component_declarations.size(); ++j) {
            auto& formula = item.formulas[j];
            if (!formula.empty()) {
              WindowItem& cell = win.AddItem("SheetCell");
              cell.SetInt("row", j + 2);
              cell.SetInt("col", i + 2);
              cell.SetString("text", '=' + formula);
            }
          }
        }

        callback(std::move(win));
      });
}
