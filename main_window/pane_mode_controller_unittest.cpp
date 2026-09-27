#include "main_window/pane_mode_controller.h"

#include "profile/page.h"
#include "profile/window_definition.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace {

// A window reduced to what the controller can observe: which panes are open,
// in open order, and what the rail was last told. Closing and activating panes
// call back into the controller the way MainWindow's view notifications do.
class FakeHost : public PaneModeHost {
 public:
  void Attach(PaneModeController& controller) { controller_ = &controller; }

  bool IsPaneOpen(std::string_view pane_type) override {
    return std::ranges::find(open, pane_type) != open.end();
  }
  bool CanOpenPane(std::string_view pane_type) override {
    return !admin_only.contains(std::string{pane_type}) || is_admin;
  }
  void ClosePane(std::string_view pane_type) override {
    std::erase(open, std::string{pane_type});
    log.push_back("close " + std::string{pane_type});
    if (controller_)
      controller_->OnPaneClosed(pane_type, /*page_closing=*/false);
  }
  void OpenPane(std::string_view pane_type) override {
    open.emplace_back(pane_type);
    log.push_back("open " + std::string{pane_type});
  }
  void FrontPane(std::string_view pane_type) override {
    log.push_back("front " + std::string{pane_type});
    active = std::string{pane_type};
    if (controller_)
      controller_->OnPaneActivated(pane_type);
  }
  std::string_view ActivePaneType() override { return active; }
  const Page& CurrentPage() override { return page; }
  std::string& PersistedModeKey() override { return mode_key; }
  void ShowModeAvailable(PaneModeId id, bool available) override {
    available_modes[id] = available;
  }
  void ShowActiveMode(std::optional<PaneModeId> id) override {
    marker = id;
    ++marker_updates;
  }

  std::vector<std::string> open;
  std::vector<std::string> log;
  std::string active;
  Page page;
  std::string mode_key;
  std::set<std::string> admin_only = {"Nodes", "Administration"};
  bool is_admin = false;

  std::map<PaneModeId, bool> available_modes;
  std::optional<PaneModeId> marker;
  int marker_updates = 0;

 private:
  PaneModeController* controller_ = nullptr;
};

class PaneModeControllerTest : public ::testing::Test {
 protected:
  PaneModeControllerTest() { host_.Attach(controller_); }

  FakeHost host_;
  PaneModeController controller_{host_};
};

TEST_F(PaneModeControllerTest, SwitchClosesBeforeOpeningAndFrontsTheSubject) {
  host_.open = {"Subsystems"};

  controller_.SetMode(PaneModeId::kObjects);

  EXPECT_EQ(host_.log,
            (std::vector<std::string>{"close Subsystems", "open Struct",
                                      "open Portfolio", "front Struct"}));
  EXPECT_EQ(host_.mode_key, "objects");
  EXPECT_EQ(host_.marker, PaneModeId::kObjects);
}

// The notifications a switch provokes must not re-derive the marker from a
// half-applied set; it is derived once, at the end.
TEST_F(PaneModeControllerTest, SwitchRefreshesTheMarkerOnlyOnce) {
  host_.open = {"Subsystems"};

  controller_.SetMode(PaneModeId::kFiles);

  EXPECT_EQ(host_.marker_updates, 1);
  EXPECT_EQ(host_.marker, PaneModeId::kFiles);
}

TEST_F(PaneModeControllerTest, AnAdminModeIsNeitherOfferedNorEnteredWithout) {
  controller_.SetMode(PaneModeId::kNodes);
  EXPECT_TRUE(host_.log.empty());
  EXPECT_TRUE(host_.mode_key.empty());

  controller_.RefreshMarker();
  EXPECT_FALSE(host_.available_modes[PaneModeId::kNodes]);
  EXPECT_TRUE(host_.available_modes[PaneModeId::kObjects]);

  host_.is_admin = true;
  controller_.SetMode(PaneModeId::kNodes);
  EXPECT_EQ(host_.mode_key, "nodes");
  EXPECT_TRUE(host_.available_modes[PaneModeId::kNodes]);
}

// A profile can carry a mode this user may not open; the sidebar must not be
// left empty with no way out.
TEST_F(PaneModeControllerTest, AnUnavailablePersistedModeFallsBackToObjects) {
  host_.mode_key = "administration";
  EXPECT_EQ(controller_.ActiveMode(), PaneModeId::kObjects);

  host_.is_admin = true;
  EXPECT_EQ(controller_.ActiveMode(), PaneModeId::kAdministration);
}

TEST_F(PaneModeControllerTest, AProfileWithNoModeInfersOneFromThePage) {
  host_.page.AddWindow(WindowDefinition{"Subsystems"});
  EXPECT_EQ(controller_.ActiveMode(), PaneModeId::kDevices);

  host_.mode_key = "not-a-mode";
  EXPECT_EQ(controller_.ActiveMode(), PaneModeId::kDevices);
}

TEST_F(PaneModeControllerTest, MarkerFallsBackToTheActivePanesMode) {
  // Objects with Portfolio closed by hand: not an exact match, but the active
  // pane still says where the operator is.
  host_.open = {"Struct"};
  host_.active = "Struct";
  controller_.RefreshMarker();
  EXPECT_EQ(host_.marker, PaneModeId::kObjects);

  // Nothing the rail owns is active: claim no mode.
  host_.active = "Graph";
  controller_.RefreshMarker();
  EXPECT_EQ(host_.marker, std::nullopt);
}

TEST_F(PaneModeControllerTest, NotificationsIgnorePanesNoModeOwns) {
  controller_.OnPaneActivated("Graph");
  controller_.OnPaneClosed("Event", /*page_closing=*/false);
  EXPECT_EQ(host_.marker_updates, 0);

  controller_.OnPaneActivated("Struct");
  EXPECT_EQ(host_.marker_updates, 1);
}

// Closing a page closes every pane in it; the marker is re-derived once the
// next page is open, not once per pane on the way down.
TEST_F(PaneModeControllerTest, PaneClosuresDuringAPageCloseAreIgnored) {
  controller_.OnPaneClosed("Struct", /*page_closing=*/true);
  EXPECT_EQ(host_.marker_updates, 0);
}

TEST_F(PaneModeControllerTest, SelectingByPaneSwitchesToItsMode) {
  EXPECT_TRUE(controller_.SelectModeForPane("Favorites"));
  EXPECT_EQ(host_.mode_key, "files");

  EXPECT_FALSE(controller_.SelectModeForPane("Event"));
  EXPECT_EQ(host_.mode_key, "files");
}

TEST_F(PaneModeControllerTest, ConformPageShowsOnlyTheActiveModesPanes) {
  host_.mode_key = "devices";
  Page page;
  page.AddWindow(WindowDefinition{"Struct"});

  controller_.ConformPage(page);

  std::set<std::string> visible;
  for (int i = 0; i < page.GetWindowCount(); ++i) {
    if (page.GetWindow(i).visible)
      visible.insert(page.GetWindow(i).type);
  }
  EXPECT_EQ(visible, std::set<std::string>{"Subsystems"});
}

}  // namespace
