#pragma once

#include "controller/controller.h"
#include "controller/controller_context.h"

#include <memory>

class WindowDefinition;

// The Roles workspace view: the RoleSet and each Role's membership.
//
// Read-only, and admin-gated through `WIN_REQUIRES_ADMIN` on the WindowInfo
// AdministrationModule registers it under — the same gate the other
// administration views use, so the client's rule and the server's (Part 18
// §5.2.1 restricts the security surface to administrators) cannot disagree.
class RolesView final : protected ControllerContext, public Controller {
 public:
  explicit RolesView(const ControllerContext& context);
  ~RolesView() override;

  // Controller
  std::unique_ptr<UiView> Init(const WindowDefinition& definition) override;

 private:
  std::shared_ptr<void> lifetime_token_ = std::make_shared<int>(0);
};
