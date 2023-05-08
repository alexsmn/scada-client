#include "components/change_password/change_password.h"

#include "base/strings/stringprintf.h"
#include "client_utils.h"
#include "components/change_password/change_password_dialog.h"
#include "model/security_node_ids.h"
#include "node_service/node_ref.h"

void ChangePassword(const ChangePasswordContext& context,
                    const scada::LocalizedText& current_password,
                    const scada::LocalizedText& new_password) {
  auto promise = context.user_.Call(security::id::UserType_ChangePassword,
                                    current_password, new_password);
  scada::BindStatusCallback(promise, [context](
                                         const scada::Status& status) {
    auto title =
        base::StringPrintf(u"Смена пароля пользователя %ls",
                           ToString16(context.user_.display_name()).c_str());
    ReportRequestResult(title, status, context.local_events_, context.profile_);
  });
}
