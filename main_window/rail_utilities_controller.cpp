#include "main_window/rail_utilities_controller.h"

#include "aui/translation.h"
#include "main_window/activity_bar_qt.h"
#include "resources/common_resources.h"

#include <optional>
#include <utility>

namespace {

// A dedicated enumeration rather than the command ids themselves, because
// those numeric values are reused across unrelated symbols (see
// resources/common_resources.h) and a rail id is compared, not dispatched.
enum class RailUtility {
  kSettings,
  kUsers,
};

constexpr int RailUtilityId(RailUtility utility) {
  return static_cast<int>(utility);
}

}  // namespace

RailUtilitiesController::RailUtilitiesController(ActivityBar& activity_bar,
                                                 ExecuteCommand execute_command,
                                                 ResolveCommand resolve_command)
    : activity_bar_{activity_bar},
      execute_command_{std::move(execute_command)},
      resolve_command_{std::move(resolve_command)} {
  // Settings opens the preferences overlay over the whole workbench and Users
  // the account list in the current page, so neither disturbs the page marker
  // (activity-rail.html).
  activity_bar_.SetUtilities(
      {ActivityBar::Utility{.utility_id = RailUtilityId(RailUtility::kSettings),
                            .label = Translate("Settings"),
                            .icon_kind = ActivityBar::Icon::kSettings},
       ActivityBar::Utility{.utility_id = RailUtilityId(RailUtility::kUsers),
                            .label = Translate("Users"),
                            .icon_kind = ActivityBar::Icon::kUsers}},
      [this](int utility_id) {
        switch (static_cast<RailUtility>(utility_id)) {
          case RailUtility::kSettings:
            execute_command_(ID_SETTINGS);
            return;
          case RailUtility::kUsers:
            execute_command_(ID_USERS_VIEW);
            return;
        }
      });
}

void RailUtilitiesController::Refresh(std::string_view active_view_type) {
  // Users is admin-only, resolved through the same command router the menus
  // use so the rail cannot offer a door the shell would refuse to open.
  activity_bar_.SetUtilityAvailable(RailUtilityId(RailUtility::kUsers),
                                    resolve_command_(ID_USERS_VIEW) != nullptr);

  // Only Users can ever be marked. Settings opens an overlay that covers the
  // rail, so a marker on its own button could never be seen while it was
  // true — the marker is a projection of what the workspace is showing, and
  // while Settings is up the workspace is not showing.
  const bool users_active = active_view_type == std::string_view{"Users"};
  activity_bar_.SetActiveUtility(
      users_active ? std::optional{RailUtilityId(RailUtility::kUsers)}
                   : std::nullopt);
}
