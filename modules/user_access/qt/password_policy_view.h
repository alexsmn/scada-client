#pragma once

#include "controller/controller.h"
#include "controller/controller_context.h"

#include <memory>

class WindowDefinition;

// The Password policy workspace view: what the server publishes on the
// UserManagement object (OPC UA Part 18 §5.2.2).
//
// Admin-gated through `WIN_REQUIRES_ADMIN` on the WindowInfo
// AdministrationModule registers it under — the properties are part of the
// security surface the server restricts to administrators (§5.2.1), so an
// operator would only get a denied read.
class PasswordPolicyView final : protected ControllerContext,
                                 public Controller {
 public:
  explicit PasswordPolicyView(const ControllerContext& context);
  ~PasswordPolicyView() override;

  // Controller
  std::unique_ptr<UiView> Init(const WindowDefinition& definition) override;

 private:
  std::shared_ptr<int> lifetime_token_ = std::make_shared<int>(0);
};
