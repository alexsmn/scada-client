#include "components/change_password/change_password.h"

#include "base/u16format.h"
#include "components/change_password/change_password_dialog.h"
#include "events/local_event_util.h"
#include "model/security_node_ids.h"
#include "node_service/node_ref.h"
#include "scada/status_promise.h"

void ChangePassword(const ChangePasswordContext& context,
                    const scada::LocalizedText& current_password,
                    const scada::LocalizedText& new_password) {
  auto promise = context.user_.scada_node().call(
      security::id::UserType_ChangePassword, current_password, new_password);
  scada::BindStatusCallback(promise, [context](const scada::Status& status) {
    auto title =
        u16format(L"Changing password for user {}",
                  ToString16(context.user_.display_name()));
    ReportRequestResult(title, status, context.local_events_, context.profile_);
  });
}
