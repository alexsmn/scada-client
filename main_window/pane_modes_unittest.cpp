#include "main_window/pane_modes.h"

#include "profile/page.h"
#include "profile/window_definition.h"

#include <gtest/gtest.h>

#include <set>
#include <string>

namespace {

// The visible left panes of `page`, as a set, so assertions read as "this mode
// is on screen" rather than depending on definition order.
std::set<std::string> VisiblePanes(const Page& page) {
  std::set<std::string> result;
  for (int i = 0; i < page.GetWindowCount(); ++i) {
    const WindowDefinition& window = page.GetWindow(i);
    if (window.visible)
      result.insert(window.type);
  }
  return result;
}

Page MakePageWithPanes(std::initializer_list<std::string_view> types) {
  Page page;
  for (std::string_view type : types)
    page.AddWindow(WindowDefinition{type});
  return page;
}

TEST(PaneModesTest, ObjectsModeListsStructAndPortfolio) {
  const PaneMode& mode = GetPaneMode(PaneModeId::kObjects);
  EXPECT_EQ(mode.key, "objects");
  EXPECT_EQ(mode.pane_types,
            (std::vector<std::string_view>{"Struct", "Portfolio"}));
  EXPECT_FALSE(mode.requires_admin);
}

TEST(PaneModesTest, AdminModesRequireAdmin) {
  EXPECT_TRUE(GetPaneMode(PaneModeId::kNodes).requires_admin);
  EXPECT_TRUE(GetPaneMode(PaneModeId::kAdministration).requires_admin);
  // Nothing else does — a mode the operator cannot reach is a hole in the rail.
  for (const PaneMode& mode : GetPaneModes()) {
    if (mode.id != PaneModeId::kNodes &&
        mode.id != PaneModeId::kAdministration) {
      EXPECT_FALSE(mode.requires_admin) << mode.key;
    }
  }
}

// The rail mode is only as reachable as its pane: MainWindow::
// IsPaneModeAvailable hides a mode whose panes resolve no command handler, and
// the router refuses a WIN_REQUIRES_ADMIN command without the Configure right.
// Naming the wrong pane here would silently show the mode to everyone.
TEST(PaneModesTest, AdministrationModeOwnsTheAdministrationPane) {
  const PaneMode& mode = GetPaneMode(PaneModeId::kAdministration);
  EXPECT_EQ(mode.key, "administration");
  EXPECT_EQ(mode.pane_types, (std::vector<std::string_view>{"Administration"}));
}

TEST(PaneModesTest, EveryOwnedPaneBelongsToExactlyOneMode) {
  std::set<std::string_view> seen;
  for (const PaneMode& mode : GetPaneModes()) {
    for (std::string_view pane_type : mode.pane_types)
      EXPECT_TRUE(seen.insert(pane_type).second) << pane_type;
  }
  EXPECT_EQ(seen.size(), GetModeOwnedPaneTypes().size());
}

// The Events pane is WIN_DOCKB, lives in the bottom dock, and is shown and
// hidden by the event auto-surface policy. The rail must never touch it.
TEST(PaneModesTest, EventsPaneIsOwnedByNoMode) {
  EXPECT_EQ(FindPaneModeOwningPaneType("Event"), nullptr);
}

TEST(PaneModesTest, FindPaneModeByKeyRejectsAnUnknownKey) {
  EXPECT_EQ(FindPaneModeByKey("objects"), &GetPaneMode(PaneModeId::kObjects));
  EXPECT_EQ(FindPaneModeByKey("databasesUsers"), nullptr);
  EXPECT_EQ(FindPaneModeByKey(""), nullptr);
}

TEST(PaneModesTest, ComputePaneModeDeltaClosesBeforeOpening) {
  const std::vector<std::string_view> open = {"Struct", "Portfolio"};
  const PaneModeDelta delta =
      ComputePaneModeDelta(GetPaneMode(PaneModeId::kFiles), open);

  EXPECT_EQ(delta.to_close,
            (std::vector<std::string_view>{"Struct", "Portfolio"}));
  // Declared order is preserved, so the mode's own subject docks first and
  // ends up fronted.
  EXPECT_EQ(delta.to_open,
            (std::vector<std::string_view>{"FileSystemView", "Favorites"}));
}

TEST(PaneModesTest, ComputePaneModeDeltaKeepsPanesTheTargetAlsoWants) {
  const std::vector<std::string_view> open = {"Struct"};
  const PaneModeDelta delta =
      ComputePaneModeDelta(GetPaneMode(PaneModeId::kObjects), open);

  EXPECT_TRUE(delta.to_close.empty());
  EXPECT_EQ(delta.to_open, (std::vector<std::string_view>{"Portfolio"}));
}

TEST(PaneModesTest, ComputePaneModeDeltaIgnoresPanesNoModeOwns) {
  const std::vector<std::string_view> open = {"Event", "Struct"};
  const PaneModeDelta delta =
      ComputePaneModeDelta(GetPaneMode(PaneModeId::kDevices), open);

  EXPECT_EQ(delta.to_close, (std::vector<std::string_view>{"Struct"}));
  EXPECT_EQ(delta.to_open, (std::vector<std::string_view>{"Subsystems"}));
}

TEST(PaneModesTest, ApplyPaneModeToPageHidesPanesOutsideTheMode) {
  Page page = MakePageWithPanes({"Struct", "Portfolio", "FileSystemView"});

  ApplyPaneModeToPage(page, GetPaneMode(PaneModeId::kFiles));

  EXPECT_EQ(VisiblePanes(page),
            (std::set<std::string>{"FileSystemView", "Favorites"}));
}

TEST(PaneModesTest, ApplyPaneModeToPageCreatesMissingPaneDefinitions) {
  Page page;

  ApplyPaneModeToPage(page, GetPaneMode(PaneModeId::kObjects));

  EXPECT_EQ(VisiblePanes(page), (std::set<std::string>{"Struct", "Portfolio"}));
}

// Hiding rather than deleting is what keeps the dock-state blob usable: it
// keys on window ids, and a pane's stored parameters live on its definition.
TEST(PaneModesTest, ApplyPaneModeToPageKeepsHiddenDefinitionsAndTheirIds) {
  Page page = MakePageWithPanes({"Struct"});
  page.GetWindow(0).AddItem("filter", std::string{"pumps"});
  const int struct_id = page.GetWindow(0).id;

  ApplyPaneModeToPage(page, GetPaneMode(PaneModeId::kDevices));
  ApplyPaneModeToPage(page, GetPaneMode(PaneModeId::kObjects));

  const WindowDefinition* restored = page.FindWindowDef(struct_id);
  ASSERT_NE(restored, nullptr);
  EXPECT_EQ(restored->type, "Struct");
  EXPECT_TRUE(restored->visible);
  EXPECT_NE(restored->FindItem("filter"), nullptr);
}

TEST(PaneModesTest, ApplyPaneModeToPageNeverTouchesTheEventsPane) {
  Page page = MakePageWithPanes({"Event", "Struct"});
  page.GetWindow(0).visible = false;

  ApplyPaneModeToPage(page, GetPaneMode(PaneModeId::kObjects));

  const WindowDefinition& events = page.GetWindow(0);
  EXPECT_EQ(events.type, "Event");
  EXPECT_FALSE(events.visible);
}

TEST(PaneModesTest, ApplyPaneModeToPageLeavesWorkspaceWindowsAlone) {
  Page page = MakePageWithPanes({"Graph", "Table", "Struct"});

  ApplyPaneModeToPage(page, GetPaneMode(PaneModeId::kDevices));

  EXPECT_TRUE(VisiblePanes(page).contains("Graph"));
  EXPECT_TRUE(VisiblePanes(page).contains("Table"));
}

TEST(PaneModesTest, InferPaneModeFromPagePicksTheBestMatch) {
  EXPECT_EQ(
      InferPaneModeFromPage(MakePageWithPanes({"FileSystemView", "Favorites"})),
      PaneModeId::kFiles);
  EXPECT_EQ(InferPaneModeFromPage(MakePageWithPanes({"Subsystems"})),
            PaneModeId::kDevices);
  EXPECT_EQ(InferPaneModeFromPage(MakePageWithPanes({"Struct", "Portfolio"})),
            PaneModeId::kObjects);
}

TEST(PaneModesTest, InferPaneModeFromPageIgnoresHiddenPanes) {
  Page page = MakePageWithPanes({"Struct", "Subsystems"});
  page.GetWindow(0).visible = false;

  EXPECT_EQ(InferPaneModeFromPage(page), PaneModeId::kDevices);
}

// A page from before the rail existed can match nothing. Objects is the
// documented tie-break, so the operator always lands somewhere sensible.
TEST(PaneModesTest, InferPaneModeFromPageDefaultsToObjects) {
  EXPECT_EQ(InferPaneModeFromPage(Page{}), PaneModeId::kObjects);
  EXPECT_EQ(InferPaneModeFromPage(MakePageWithPanes({"Graph", "Table"})),
            PaneModeId::kObjects);
}

}  // namespace
