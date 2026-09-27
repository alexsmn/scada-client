#include "user_access/qt/user_access_selection_panel.h"

#include "aui/translation.h"
#include "controller/qt/selection_panel_registry.h"
#include "controller/selection_model.h"
#include "model/security_node_ids.h"
#include "node_service/node_util.h"
#include "user_access/qt/user_access_panel.h"

#include <memory>
#include <string>

namespace {

// Adapts UserAccessPanel to the shell's selection-panel contract.
class UserAccessSelectionPanel final : public SelectionPanel {
 public:
  UserAccessSelectionPanel(UserAccessPanel& panel,
                           const SelectionPanelContext& context)
      : panel_{panel}, context_{context} {}

  std::string object_name() const override { return "UserAccessDock"; }
  std::u16string title() const override { return Translate("Access rights"); }
  QWidget& widget() override { return panel_; }

  void ShowSelection(const SelectionModel* selection) override {
    // A single user-node selection fills the RBAC panel; anything else
    // clears. Without a node service and an attribute service there is no way
    // to read what the Roles grant, and the panel must not fall back to
    // assuming a map — so the selection clears rather than showing an invented
    // breakdown.
    if (selection && !selection->empty() && !selection->multiple() &&
        context_.node_service && context_.attribute_service &&
        IsInstanceOf(selection->node(), scada::security::id::UserType)) {
      // The node carries the account's NAME; the panel reads the Roles
      // themselves from the RoleSet, and what they grant from the server's
      // published RolePermissions.
      panel_.ShowUser(selection->node(), *context_.node_service,
                      *context_.attribute_service, context_.executor);
    } else {
      panel_.Clear();
    }
  }

 private:
  UserAccessPanel& panel_;
  const SelectionPanelContext context_;
};

std::unique_ptr<SelectionPanel> CreatePanel(
    const SelectionPanelContext& context) {
  UserAccessPanel* panel = MakeUserAccessPanel();
  if (!panel)
    return nullptr;
  return std::make_unique<UserAccessSelectionPanel>(*panel, context);
}

}  // namespace

void RegisterUserAccessSelectionPanel(SelectionPanelRegistry& registry) {
  registry.Register(&CreatePanel);
}
