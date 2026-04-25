#include "components/change_password/change_password.h"

#include "base/awaitable.h"
#include "base/awaitable_promise.h"
#include "base/u16format.h"
#include "components/change_password/change_password_dialog.h"
#include "events/local_event_util.h"
#include "model/security_node_ids.h"
#include "node_service/node_ref.h"
#include "scada/status_exception.h"

namespace {

Awaitable<void> ReportPasswordChangeResultAsync(
    std::shared_ptr<Executor> executor,
    promise<> call_promise,
    std::u16string title,
    LocalEvents& local_events,
    const Profile& profile) {
  scada::Status status = scada::StatusCode::Good;
  try {
    co_await AwaitPromise(NetExecutorAdapter{std::move(executor)},
                          std::move(call_promise));
  } catch (...) {
    status = scada::GetExceptionStatus(std::current_exception());
  }

  ReportRequestResult(title, status, local_events, profile);
}

}  // namespace

void ChangePassword(const ChangePasswordContext& context,
                    const scada::LocalizedText& current_password,
                    const scada::LocalizedText& new_password) {
  auto call_promise = context.user_.scada_node().call(
      security::id::UserType_ChangePassword, current_password, new_password);
  CoSpawn(context.executor_,
          [executor = context.executor_,
           call_promise = std::move(call_promise),
           title = u16format(L"Changing password for user {}",
                             ToString16(context.user_.display_name())),
           &local_events = context.local_events_,
           &profile = context.profile_]() mutable -> Awaitable<void> {
            co_await ReportPasswordChangeResultAsync(
                std::move(executor), std::move(call_promise), std::move(title),
                local_events, profile);
          });
}
