#include "user_access/qt/roles_view.h"

#include "base/awaitable.h"
#include "node_service/node_service.h"
#include "user_access/qt/roles_grid_panel.h"
#include "user_access/role_membership.h"

#include <QPointer>

// The WindowInfo and the controller registration live in the administration
// module, not here: a REGISTER_CONTROLLER in a static-library TU that nothing
// else references is dropped by the linker, so the view would silently not
// exist. AdministrationModule is constructed at startup and registers it.

RolesView::RolesView(const ControllerContext& context)
    : ControllerContext{context} {}

RolesView::~RolesView() = default;

std::unique_ptr<UiView> RolesView::Init(const WindowDefinition& definition) {
  RolesGridPanel* panel = MakeRolesGridPanel();
  if (!panel) {
    return nullptr;
  }

  // Populate off the construction path; a QPointer guards a late completion
  // against a destroyed panel.
  CoSpawn(executor_, [executor = executor_, &node_service = node_service_,
                      panel_ptr = QPointer<RolesGridPanel>{panel}]() mutable
          -> Awaitable<void> {
    auto roles = co_await ReadRoleMemberships(executor, node_service);
    if (panel_ptr) {
      panel_ptr->ShowRoles(roles);
    }
    co_return;
  });

  return std::unique_ptr<UiView>{panel};
}
