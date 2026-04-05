#include "page_commands.h"

#include "aui/dialog_service_mock.h"
#include "controller/command_registry.h"
#include "core/global_command_context.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/main_window_mock.h"
#include "profile/profile.h"

#include <gmock/gmock.h>

using namespace testing;

class PageCommandsTest : public Test {
 protected:
  BasicCommandRegistry<GlobalCommandContext> commands_;
  Profile profile_;

  StrictMock<MockFunction<std::unique_ptr<MainWindow>(int window_id)>>
      main_window_factory_;

  StrictMock<MockFunction<void()>> quit_handler_;

  MainWindowManager main_window_manager_{{profile_,
                                          main_window_factory_.AsStdFunction(),
                                          quit_handler_.AsStdFunction()}};

  StrictMock<MockMainWindow> main_window_;
  StrictMock<MockDialogService> dialog_service_;

  GlobalCommandContext command_context_{.main_window = main_window_,
                                      .dialog_service = dialog_service_};

  PageCommands page_commands_{{commands_, profile_, main_window_manager_}};
};

TEST_F(PageCommandsTest, DeletePage) {
  auto* command = commands_.FindCommand(ID_PAGE_DELETE);
  ASSERT_THAT(command, NotNull());

  EXPECT_CALL(main_window_, DeleteCurrentPage());

  command->execute_handler(command_context_);
}
