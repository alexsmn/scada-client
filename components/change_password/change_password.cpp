#include "components/change_password/change_password.h"

#include "base/strings/stringprintf.h"
#include "client_utils.h"
#include "node_service/node_ref.h"
#include "components/change_password/change_password_dialog.h"
#include "core/node_management_service.h"

void ChangePassword(const ChangePasswordContext& context,
                    const scada::LocalizedText& current_password,
                    const scada::LocalizedText& new_password) {
  context.node_management_service_.ChangeUserPassword(
      context.user_.node_id(), current_password, new_password,
      [context](const scada::Status& status) {
        base::string16 title = base::StringPrintf(
            L"Смена пароля пользователя %ls",
            ToString16(context.user_.display_name()).c_str());
        ReportRequestResult(title, status, context.local_events_,
                            context.profile_);
      });
}
