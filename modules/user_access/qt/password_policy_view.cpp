#include "user_access/qt/password_policy_view.h"

#include "base/awaitable.h"
#include "node_service/node_service.h"
#include "user_access/password_policy.h"
#include "user_access/qt/password_policy_panel.h"

#include <QPointer>

PasswordPolicyView::PasswordPolicyView(const ControllerContext& context)
    : ControllerContext{context} {}

PasswordPolicyView::~PasswordPolicyView() = default;

std::unique_ptr<UiView> PasswordPolicyView::Init(
    const WindowDefinition& definition) {
  PasswordPolicyPanel* panel = MakePasswordPolicyPanel();
  if (!panel) {
    return nullptr;
  }

  CoSpawn(executor_, [executor = executor_, &node_service = node_service_,
                      panel_ptr = QPointer<PasswordPolicyPanel>{panel}]() mutable
          -> Awaitable<void> {
    auto policy = co_await ReadPasswordPolicy(executor, node_service);
    if (panel_ptr) {
      panel_ptr->ShowPolicy(policy);
    }
    co_return;
  });

  return std::unique_ptr<UiView>{panel};
}
