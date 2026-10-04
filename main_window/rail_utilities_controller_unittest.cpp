#include "main_window/rail_utilities_controller.h"

#include "aui/test/app_environment.h"
#include "controller/command_handler.h"
#include "main_window/activity_bar_qt.h"
#include "resources/common_resources.h"

#include <QAction>
#include <QWidget>

#include <gmock/gmock.h>

#include <vector>

using namespace testing;

namespace {

class RailUtilitiesControllerTest : public Test {
 protected:
  RailUtilitiesController MakeController() {
    return RailUtilitiesController{
        bar_, [this](unsigned command_id) { executed_.push_back(command_id); },
        [this](unsigned command_id) -> CommandHandler* {
          return command_id == ID_USERS_VIEW && users_resolve_ ? &handler_
                                                               : nullptr;
        }};
  }

  // The rail's action carrying `label` as its tooltip.
  QAction* Utility(const char* label) const {
    for (QAction* action : bar_.actions()) {
      if (action->toolTip() == QString::fromUtf8(label))
        return action;
    }
    return nullptr;
  }

  AppEnvironment app_env_;
  QWidget parent_;
  ActivityBar bar_{&parent_, {}, [](PaneModeId) {}};
  CommandHandler handler_;
  bool users_resolve_ = true;
  std::vector<unsigned> executed_;
};

TEST_F(RailUtilitiesControllerTest, EachUtilityRunsItsCommand) {
  RailUtilitiesController controller = MakeController();

  ASSERT_NE(Utility("Settings"), nullptr);
  ASSERT_NE(Utility("Users"), nullptr);
  Utility("Settings")->trigger();
  Utility("Users")->trigger();

  EXPECT_THAT(executed_, ElementsAre(ID_SETTINGS, ID_USERS_VIEW));
}

// Users is offered only when its command resolves for this session — the
// rail must not offer a door the shell would refuse to open.
TEST_F(RailUtilitiesControllerTest, UsersIsHiddenWhenItsCommandDoesNotResolve) {
  RailUtilitiesController controller = MakeController();

  users_resolve_ = false;
  controller.Refresh({});
  EXPECT_FALSE(Utility("Users")->isVisible());
  EXPECT_TRUE(Utility("Settings")->isVisible());

  users_resolve_ = true;
  controller.Refresh({});
  EXPECT_TRUE(Utility("Users")->isVisible());
}

// The marker follows the workspace: Users is marked exactly while the Users
// view is active, and Settings is never marked.
TEST_F(RailUtilitiesControllerTest, UsersIsMarkedWhileItsViewIsActive) {
  RailUtilitiesController controller = MakeController();

  controller.Refresh("Users");
  EXPECT_TRUE(Utility("Users")->isChecked());
  EXPECT_FALSE(Utility("Settings")->isChecked());

  controller.Refresh("Graph");
  EXPECT_FALSE(Utility("Users")->isChecked());

  controller.Refresh({});
  EXPECT_FALSE(Utility("Users")->isChecked());
}

}  // namespace
