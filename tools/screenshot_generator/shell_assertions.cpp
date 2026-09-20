// Assertions about the running shell's own chrome — the status strip, the
// context bar, the installed translation, and that a page of the heaviest
// view boots at all.
//
// Each drives the ScreenshotGenerator fixture and writes no PNG: they pin
// properties of the live widget tree that a rendered image cannot express and
// that check_screenshots.py — existence, dimensions, pairwise distinctness,
// never text — is structurally unable to see.

#include "screenshot_fixture.h"

#include "aui/translation.h"
#include "authenticated_attribute_service.h"
#include "base/utf_convert.h"
#include "common/format.h"
#include "configuration/objects/visible_node_model.h"
#include "events/qt/event_filter_bar.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "model/security_node_ids.h"
#include "modules/limits/limit_model.h"
#include "modules/transmission/transmission_devices.h"
#include "modules/write/write_model.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_fetch_status.h"
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "null_task_manager.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "scada/qualifier.h"
#include "scada/standard_node_ids.h"
#include "scada/status.h"
#include "scada/variant.h"
#include "screenshot_wait.h"
#include "services/device_state_notifier.h"
#include "user_access/role_membership.h"

#include <QApplication>
#include <QLabel>
#include <QLayout>
#include <QStatusBar>
#include <QString>
#include <QToolBar>
#include <QWidget>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

namespace {

using scada::screenshot_generator::FixtureAccountsWith;
using scada::screenshot_generator::FixtureConfig;
using scada::screenshot_generator::ScreenshotGenerator;
using scada::screenshot_generator::ShowMainWindowForMenuCapture;
using scada::screenshot_generator::WaitForAwaitable;
using scada::screenshot_generator::WaitForPendingNodeLoads;

}  // namespace


// The status strip must name the signed-in operator, not just their role.
// `LocalSessionService` reports a null user id by default, and
// `UserStatusProvider::GetText()` falls back to the bare role label when the
// id resolves to a node with no display name — so the user cell rendered
// "Администратор" alone and the fixture looked signed in as nobody. Asserted
// against the live widget tree rather than the rendered pixels, because this
// capture has no `screenshot_data.json` spec for check_screenshots.py to
// verify dimensions against.
TEST_F(ScreenshotGenerator, StatusBarNamesTheSignedInUser) {
  MainWindow::SetHideForTesting(false);

  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"Struct"});
    profile.AddPage(page);
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  QMainWindow* qmain = ShowMainWindowForMenuCapture(app_);
  ASSERT_NE(qmain, nullptr);

  auto* status_bar = qmain->findChild<QStatusBar*>();
  ASSERT_NE(status_bar, nullptr);

  // The display name of the account `session_user_node_id` names. Read from
  // the fixture rather than written here, so renaming the account cannot leave
  // this test asserting a name nothing renders.
  QString expected_user;
  for (const auto& node : FixtureConfig().json.at("nodes").as_array()) {
    const auto& object = node.as_object();
    if (scada::NodeIdFromScadaString(
            std::string_view(object.at("id").as_string())) ==
        FixtureConfig().session_user_node_id) {
      expected_user = QString::fromStdString(
          std::string(object.at("display_name").as_string()));
      break;
    }
  }
  ASSERT_FALSE(expected_user.isEmpty())
      << "session_user_node_id names no fixture node with a display_name";

  bool found = false;
  for (const QLabel* pane : status_bar->findChildren<QLabel*>()) {
    if (pane->text().contains(expected_user)) {
      found = true;
      break;
    }
  }
  EXPECT_TRUE(found) << "no status-bar pane names the signed-in user "
                     << expected_user.toStdString();

  MainWindow::SetHideForTesting(true);
}

// The context bar's outer content must stand off the window edge.
//
// A QToolBar contributes only its style frame horizontally -- 4px on the left
// and 7px on the right, measured on macOS -- and all three slots zeroed their
// horizontal margins, so the breadcrumb's first glyph started at x=4 and the
// last alarm tile ended 7px from the other edge. Every other row in the window
// stands off it: the menu bar by its own style, the status strip by its cells'
// margins. `operator-shell.html` gives `.topbar` a `padding: 0 12px`.
//
// Asserted on the live widget tree rather than on pixels, for the same reason
// the test above is: `check_screenshots.py` verifies existence, dimensions and
// distinctness and is structurally unable to see where the ink starts. The
// assertion is "more than the bare frame", not an exact figure -- the inset is
// a style metric, so it moves with the platform, the DPI and the OS text size,
// and pinning a number here would make this test a machine's rather than a
// rule's.
TEST_F(ScreenshotGenerator, ContextBarKeepsItsContentOffTheWindowEdge) {
  MainWindow::SetHideForTesting(false);

  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"Struct"});
    profile.AddPage(page);
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  QMainWindow* qmain = ShowMainWindowForMenuCapture(app_);
  ASSERT_NE(qmain, nullptr);

  auto* context_bar = qmain->findChild<QToolBar*>(QStringLiteral("ContextBar"));
  ASSERT_NE(context_bar, nullptr);

  auto* left_slot =
      context_bar->findChild<QWidget*>(QStringLiteral("contextBarLeftSlot"));
  auto* right_slot =
      context_bar->findChild<QWidget*>(QStringLiteral("contextBarRightSlot"));
  auto* centre_slot =
      context_bar->findChild<QWidget*>(QStringLiteral("contextBarCentreSlot"));
  ASSERT_NE(left_slot, nullptr);
  ASSERT_NE(right_slot, nullptr);
  ASSERT_NE(centre_slot, nullptr);

  EXPECT_GT(left_slot->layout()->contentsMargins().left(), 0)
      << "the breadcrumb sits against the window's left edge";
  EXPECT_GT(right_slot->layout()->contentsMargins().right(), 0)
      << "the alarm cluster sits against the window's right edge";

  // The inset is the BAR's, so it belongs to the two slots that touch the
  // window. Padding the centre one would narrow the command field to buy
  // nothing -- it is bounded by its neighbours.
  EXPECT_EQ(centre_slot->layout()->contentsMargins().left(), 0);
  EXPECT_EQ(centre_slot->layout()->contentsMargins().right(), 0);

  MainWindow::SetHideForTesting(true);
}

// Operator-facing text that shared code produces — status-code descriptions,
// data-quality flags, boolean value labels — reaches the UI through
// `scada::TranslateUiText`, whose installed translator is the client's
// `Translate()`. That looks up in the *empty* translation context, deliberately
// (client/aui/qt/translation_qt.cpp says why), so those entries have to sit in
// the empty context of `client_ru.ts`. Filed under the class that happens to
// display them, the lookup misses and the English source renders inside the
// Russian UI — with the translation present and correct all along.
//
// Nothing else in this pipeline can see that. The capture still renders, still
// matches its spec dimensions, and still differs from no other capture, so
// `check_screenshots.py` passes; it surfaces only as an unexplained image diff
// against the tracked gallery. Task 376 was exactly this, and it had swallowed
// the whole `core/scada/status.cpp` table plus `qualifier.cpp` and
// `variant.cpp` — 64 entries.
//
// One string per source, asserted to have left ASCII behind rather than
// asserted equal to its Russian: Cyrillic literals do not belong in this
// tree's sources, and the fallback this guards against is by definition the
// ASCII source text.
TEST_F(ScreenshotGenerator, TranslatedUiTextResolvesToRussian) {
  const auto is_translated = [](std::u16string_view text) {
    return !text.empty() && std::any_of(text.begin(), text.end(),
                                        [](char16_t c) { return c > 0x7f; });
  };

  // core/scada/status.cpp — rendered in the object table's status column.
  const std::u16string status =
      ::ToString16(scada::StatusCode::Bad_WrongNodeId);
  EXPECT_TRUE(is_translated(status))
      << "status description fell back to its English source: "
      << QString::fromStdU16String(status).toStdString();

  // core/scada/qualifier.cpp — the quality strip beside a value.
  const std::u16string quality =
      ::ToString16(scada::Qualifier().set_sporadic(true));
  EXPECT_TRUE(is_translated(quality))
      << "quality flag fell back to its English source: "
      << QString::fromStdU16String(quality).toStdString();

  // core/scada/variant.cpp — how a boolean value prints.
  const std::u16string boolean = scada::Variant::TrueLabel();
  EXPECT_TRUE(is_translated(boolean))
      << "boolean label fell back to its English source: "
      << QString::fromStdU16String(boolean).toStdString();
}

// Regression test for a stack overflow that fires during `app_.Start()`
// when a page containing a Struct (tree) window is loaded on top of the
// in-memory address space.
//
// The fixture wires the real `v1::NodeServiceImpl` and
// `NodeServiceTreeImpl` over synchronous Local* services that complete
// fetch callbacks in the caller's stack frame. During boot the tree
// model opens the root; each `ConfigurationTreeNode` ctor calls
// `node_.Fetch(NodeOnly)`; the fetch completes synchronously, fires
// `OnNodeChildrenChanged`, which re-enters `UpdateChildTreeNodes`, which
// creates more `ConfigurationTreeNode` children, each calling `Fetch()`
// again — and so on until the stack runs out. In production (gRPC)
// fetches return async over a socket, so the chain stays shallow and
// the bug never surfaces.
//
// This test is the smallest repro: only a Struct window on the page,
// no screenshots, no dialogs. The bug fix should let `app_.Start()`
// return normally and leave one main window open.
TEST_F(ScreenshotGenerator, BootWithStructPageDoesNotOverflowStack) {
  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"Struct"});
    profile.AddPage(page);
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());

  // Let the initial address-space fetch cascade complete.
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  // If the fetch cascade overflowed the stack the process would have
  // aborted before this line — reaching here means the recursion is
  // bounded.
  EXPECT_EQ(app_.main_window_manager().main_windows().size(), 1u);
}
