#include "device_diagnostics/qt/device_diagnostics_selection_panel.h"

#include "aui/translation.h"
#include "base/awaitable.h"
#include "controller/command_handler.h"
#include "controller/qt/selection_panel_registry.h"
#include "controller/selection_model.h"
#include "device_diagnostics/device_diagnostics_fetch.h"
#include "device_diagnostics/qt/device_diagnostics_panel.h"
#include "model/devices_node_ids.h"
#include "node_service/node_util.h"
#include "resources/common_resources.h"

#include <memory>
#include <string>
#include <utility>

namespace {

// A device-wide action that reuses a selection-scoped device command, resolved
// against the active selection exactly like the toolbar/menu path.
DiagnosticAction MakeCommandAction(const SelectionPanelContext& context,
                                   unsigned command_id,
                                   std::u16string label) {
  auto resolve = context.resolve_command;
  return DiagnosticAction{
      .label = std::move(label),
      .execute =
          [resolve, command_id] {
            CommandHandler* handler = resolve(command_id);
            if (handler && handler->IsCommandEnabled(command_id))
              handler->ExecuteCommand(command_id);
          },
      .is_enabled =
          [resolve, command_id] {
            CommandHandler* handler = resolve(command_id);
            return handler && handler->IsCommandEnabled(command_id);
          },
      // These are gated by the selection, not by a right, so the reason an
      // operator can act on is to change what is selected.
      .disabled_reason = Translate("Not available for the current selection")};
}

// Adapts DeviceDiagnosticsPanel to the shell's selection-panel contract.
class DeviceDiagnosticsSelectionPanel final : public SelectionPanel {
 public:
  explicit DeviceDiagnosticsSelectionPanel(DeviceDiagnosticsPanel& panel)
      : panel_{panel} {}

  std::string object_name() const override { return "DeviceDiagnosticsDock"; }
  std::u16string title() const override {
    return Translate("Device diagnostics");
  }
  QWidget& widget() override { return panel_; }

  void ShowSelection(const SelectionModel* selection) override {
    // Only a single device selection carries diagnostics; anything else
    // clears the panel.
    if (selection && !selection->empty() && !selection->multiple() &&
        IsInstanceOf(selection->node(), scada::devices::id::DeviceType)) {
      panel_.ShowDevice(selection->node(), selection->timed_data_service());
    } else {
      panel_.Clear();
    }
  }

 private:
  DeviceDiagnosticsPanel& panel_;
};

std::unique_ptr<SelectionPanel> CreatePanel(
    const SelectionPanelContext& context) {
  // "Reconnect now" is NOT one of the command actions. It is the protocol
  // registry's link action: an OPC UA Method on the device's parent LINK
  // (ADR 0007), supplied per device by the panel itself, because only some
  // protocols have one and only some devices have a link.
  DeviceDiagnosticsPanelContext panel_context;
  panel_context.actions.push_back(MakeCommandAction(
      context, ID_OPEN_DEVICE_METRICS, Translate("Metrics trend")));
  panel_context.actions.push_back(
      MakeCommandAction(context, ID_OPEN_EVENTS, Translate("Open log")));
  panel_context.call_link_method = context.call_node_method;
  panel_context.can_call = context.has_call_permission;
  // The panel's rows come from the device's children and its parent link,
  // which a selection does not make resident — and an unfetched parent costs
  // the operator the Reconnect action on a device whose link is down.
  panel_context.load = [executor = context.executor](
                           const NodeRef& device,
                           std::function<void()> redraw) {
    CoSpawn(executor,
            [device, redraw = std::move(redraw)]() -> Awaitable<void> {
              co_await FetchDeviceDiagnostics(device);
              redraw();
            });
  };

  DeviceDiagnosticsPanel* panel =
      MakeDeviceDiagnosticsPanel(std::move(panel_context));
  if (!panel)
    return nullptr;
  return std::make_unique<DeviceDiagnosticsSelectionPanel>(*panel);
}

}  // namespace

void RegisterDeviceDiagnosticsSelectionPanel(SelectionPanelRegistry& registry) {
  registry.Register(&CreatePanel);
}
