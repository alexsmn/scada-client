#include "main_window/main_window_util.h"

#include "controller/command_ui_registry.h"
#include "controller/main_menu_id.h"
#include "resources/common_resources.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

namespace {

// The "Empty" group of the workspace tab strip's `+`
// (docs/product/ui-mockups/screens/shell-chrome.html): the commands that open a
// view with no selection at all.

std::vector<std::u16string> Titles(
    const std::vector<MenuContribution>& contributions) {
  std::vector<std::u16string> titles;
  for (const MenuContribution& contribution : contributions) {
    titles.push_back(contribution.title);
  }
  return titles;
}

bool Contains(const std::vector<MenuContribution>& contributions,
              unsigned command_id) {
  return std::ranges::any_of(contributions,
                             [command_id](const MenuContribution& c) {
                               return c.command_id == command_id;
                             });
}

auto AlwaysEnabled() {
  return [](unsigned) { return true; };
}

// These are the ids the real modules register under the Graph and Table menus.
// ID_GRAPH_VIEW / ID_TABLE_VIEW / ID_TIMED_DATA_VIEW are *window* ids, so each
// opens a view; ID_OPEN_GROUP_TABLE is a selection command that sits in the
// same menu and names no window.
void RegisterTheRealShape(UiCommandRegistry& registry) {
  registry.AddMenuItem({.menu_id = MainMenuId::Graph,
                        .order = 100,
                        .command_id = ID_GRAPH_VIEW,
                        .title = u"New Graph"});
  registry.AddMenuItem({.menu_id = MainMenuId::Table,
                        .order = 100,
                        .command_id = ID_TABLE_VIEW,
                        .title = u"New Table"});
  registry.AddMenuItem({.menu_id = MainMenuId::Table,
                        .order = 120,
                        .command_id = ID_TIMED_DATA_VIEW,
                        .title = u"New Data Table"});
  registry.AddMenuItem({.menu_id = MainMenuId::Table,
                        .order = 200,
                        .command_id = ID_OPEN_GROUP_TABLE,
                        .title = u"Group Table"});
}

TEST(FindEmptyViewCommandsTest, OffersTheThreeWindowOpenersInMenuOrder) {
  UiCommandRegistry registry;
  RegisterTheRealShape(registry);

  const std::vector<MenuContribution> empty =
      FindEmptyViewCommands(registry, AlwaysEnabled());

  EXPECT_EQ(Titles(empty), (std::vector<std::u16string>{
                               u"New Graph", u"New Table", u"New Data Table"}));
}

// The test that earns the window-id rule: `Group Table` is registered in the
// Table menu beside the two empty-view openers and must not be offered as one.
// It opens a view over the *selected* node's parent group, so in a menu whose
// section says "Empty" it would be a command that cannot do what the section
// promises.
TEST(FindEmptyViewCommandsTest, LeavesOutASelectionCommandInTheSameMenu) {
  UiCommandRegistry registry;
  RegisterTheRealShape(registry);

  const std::vector<MenuContribution> empty =
      FindEmptyViewCommands(registry, AlwaysEnabled());

  EXPECT_FALSE(Contains(empty, ID_OPEN_GROUP_TABLE));
}

// Nothing is hard-coded: a command registered in one of the two menus whose id
// is a window id is offered, whoever registered it.
TEST(FindEmptyViewCommandsTest, PicksUpANewlyRegisteredWindowOpener) {
  UiCommandRegistry registry;
  RegisterTheRealShape(registry);
  registry.AddMenuItem({.menu_id = MainMenuId::Graph,
                        .order = 150,
                        .command_id = ID_SUMMARY_VIEW,
                        .title = u"New Summary"});

  const std::vector<MenuContribution> empty =
      FindEmptyViewCommands(registry, AlwaysEnabled());

  EXPECT_TRUE(Contains(empty, ID_SUMMARY_VIEW));
}

// A command the session cannot run is left out rather than offered dead — the
// caller owns resolution, so the predicate is where that judgement lands.
TEST(FindEmptyViewCommandsTest, LeavesOutACommandThePredicateRejects) {
  UiCommandRegistry registry;
  RegisterTheRealShape(registry);

  const std::vector<MenuContribution> empty = FindEmptyViewCommands(
      registry,
      [](unsigned command_id) { return command_id != ID_TIMED_DATA_VIEW; });

  EXPECT_FALSE(Contains(empty, ID_TIMED_DATA_VIEW));
  EXPECT_TRUE(Contains(empty, ID_TABLE_VIEW));
}

// Every contribution rejected means an empty group, which the `+` menu renders
// as no section at all rather than a header over nothing.
TEST(FindEmptyViewCommandsTest, ReturnsNothingWhenEveryCommandIsRejected) {
  UiCommandRegistry registry;
  RegisterTheRealShape(registry);

  EXPECT_TRUE(
      FindEmptyViewCommands(registry, [](unsigned) { return false; }).empty());
}

// A menu this group does not draw from contributes nothing, however many
// window openers it holds. The group is the Graph and Table menus' by design:
// `Events` and `Summary` have no "open an empty one" command to offer.
TEST(FindEmptyViewCommandsTest, IgnoresOtherMenus) {
  UiCommandRegistry registry;
  registry.AddMenuItem({.menu_id = MainMenuId::More,
                        .order = 100,
                        .command_id = ID_SUMMARY_VIEW,
                        .title = u"New Summary"});

  EXPECT_TRUE(FindEmptyViewCommands(registry, AlwaysEnabled()).empty());
}

}  // namespace
