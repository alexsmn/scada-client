#include "client/components/main/client_commands.h"

#include "client/client_application.h"
#include "client/client_utils.h"
#include "client/common_resources.h"
#include "client/components/main/main_window_util.h"
#include "client/services/profile.h"

namespace client {

void ExecuteCommand(MainWindow* main_window, unsigned command_id) {
}

void OpenRecordEditor(MainWindow* main_window, const NodeRef& node) {
  ::OpenView(main_window, PrepareWindowDefinitionForOpen(node, ID_PROPERTY_VIEW));
}

} // namespace client
