// Assertions the screenshot fixture makes without capturing anything.
//
// Every test here drives the same `ScreenshotGenerator` fixture as the capture
// sweep in `main.cpp`, but writes no PNG: each pins a property of the fixture
// or of the live widget tree that a rendered image cannot express, and that
// `check_screenshots.py` — which asserts existence, dimensions and pairwise
// distinctness, never text — is structurally unable to see. They live apart
// from the sweep because they share only the fixture with it, and because the
// sweep is the expensive half: these run in milliseconds and are the first
// thing to read when a capture looks wrong.

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
#include "node_service/node_ref.h"
#include "node_service/node_util.h"
#include "null_task_manager.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "scada/qualifier.h"
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

// Verifies the event journal's Area filter populates at runtime: the same
// enumeration the filter bar drives (`BrowseEventAreas`), run against the real
// `v1::NodeServiceImpl` over the fixture address space, returns the operator's
// top-level area groupings and drops leaf data items.
TEST_F(ScreenshotGenerator, EventFilterBarEnumeratesAreas) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  NodeService& node_service = app_.node_service();
  std::vector<EventAreaEntry> areas =
      WaitForAwaitable(executor_, BrowseEventAreas(node_service));

  // The Area dropdown is populated from this list — it must not be empty.
  ASSERT_FALSE(areas.empty());

  // Every enumerated area is a named object grouping, never a leaf data item —
  // that is the level the operator filters the journal by.
  for (const EventAreaEntry& area : areas) {
    EXPECT_FALSE(area.name.empty());
    NodeRef node = node_service.GetNode(area.node_id);
    EXPECT_FALSE(IsInstanceOf(node, scada::data_items::id::DataItemType))
        << "an area must not be a leaf data item";
  }

  // The DataItems root also holds loose top-level data items; the enumeration
  // partitions its Organizes children exactly into areas + excluded leaves.
  std::vector<NodeRef> children = node_service.GetTargets(
      scada::data_items::id::DataItems, scada::id::Organizes, /*forward=*/true);
  size_t leaf_count = 0;
  for (NodeRef& child : children) {
    if (IsInstanceOf(child, scada::data_items::id::DataItemType))
      ++leaf_count;
  }
  EXPECT_EQ(areas.size() + leaf_count, children.size());
}

// Verifies the Transmission view's destination rail populates at runtime: the
// same walk the rail drives (`BrowseTransmissionDevices`), run against the
// real `v1::NodeServiceImpl` over the fixture address space, finds every
// transmission-capable fixture device with its rule count — the plain Modbus
// device (a valid, rule-less destination) and both retransmission devices.
TEST_F(ScreenshotGenerator, DestinationRailEnumeratesTransmissionDevices) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  NodeService& node_service = app_.node_service();
  std::vector<TransmissionDeviceEntry> devices = WaitForAwaitable(
      executor_, BrowseTransmissionDevices(
                     node_service.GetNode(scada::devices::id::Devices)));

  ASSERT_EQ(devices.size(), 3u);
  EXPECT_EQ(devices[0].node_id, NodeIdFromScadaString("TS.104"));
  EXPECT_EQ(devices[0].rule_count, 0);
  EXPECT_EQ(devices[1].node_id, NodeIdFromScadaString("TS.702"));
  EXPECT_EQ(devices[1].rule_count, 4);
  EXPECT_EQ(devices[2].node_id, NodeIdFromScadaString("TS.703"));
  EXPECT_EQ(devices[2].rule_count, 2);
  for (const TransmissionDeviceEntry& device : devices)
    EXPECT_FALSE(device.name.empty());
}

// Role membership must survive the real node model, not just a static fake.
//
// `ReadRoleMemberships` resolves each rule's Criteria through the TYPE's
// property declarations, so the type chain has to be fetched first. It was
// not, and against the real address space both property lookups returned a
// null NodeRef — every Role came back with no members, which is what
// `roles.png` and the managed `users-admin.png` documented. The module's own
// unit tests could not see it: a StaticNodeService resolves the declaration
// whether or not anything fetched the type.
TEST_F(ScreenshotGenerator, RolesEnumerateTheirMembers) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  const std::optional<std::vector<RoleMembership>> roles = WaitForAwaitable(
      executor_, ReadRoleMemberships(executor_, app_.node_service(),
                                     authenticated_attribute_service_));
  ASSERT_TRUE(roles.has_value());

  const auto members_of = [&roles](scada::WellKnownRole role) {
    const scada::NodeId id = scada::WellKnownRoleId(role);
    auto i = std::ranges::find(*roles, id, &RoleMembership::node_id);
    return i != roles->end() ? i->members : std::vector<std::u16string>{};
  };

  const std::vector<std::u16string> control =
      FixtureAccountsWith(scada::AccessRight::kControl);
  const std::vector<std::u16string> configure =
      FixtureAccountsWith(scada::AccessRight::kConfigure);
  ASSERT_FALSE(control.empty());
  ASSERT_FALSE(configure.empty());

  EXPECT_THAT(members_of(scada::WellKnownRole::kOperator),
              testing::UnorderedElementsAreArray(control));
  EXPECT_THAT(members_of(scada::WellKnownRole::kConfigureAdmin),
              testing::UnorderedElementsAreArray(configure));
}

// The limits dialog draws whatever the target node's four limit-band
// properties hold, and nothing else — so a node without them captures a
// dialog whose every field is blank. `limits.png` shipped that way against
// the manual, which shows all four populated.
//
// Locking the values down here rather than only in the fixture: the capture
// pipeline has no assertion that would notice the fields going empty again
// (check_screenshots.py verifies a PNG exists with the right dimensions,
// which an empty dialog satisfies), and the properties are easy to drop while
// editing the fixture for some other capture. Driving the real LimitModel,
// not the raw property reads, keeps the residency requirement in scope: the
// bands are property children, and an unfetched node reads them all as null.
TEST_F(ScreenshotGenerator, LimitDialogNodeCarriesItsBands) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  const scada::NodeId node_id = FixtureConfig().dialog_analog_node_id;
  ASSERT_TRUE(scada::screenshot_generator::FetchNodesResident(
      executor_, app_.node_service(),
      std::span<const scada::NodeId>{&node_id, 1}));

  NullTaskManager task_manager;
  LimitModel model{LimitDialogContext{
      executor_, app_.node_service().GetNode(node_id), task_manager}};

  EXPECT_FALSE(model.GetSourceTitle().empty())
      << "the dialog's source title is the node display name";

  const LimitModel::Limits limits = model.GetLimits();
  EXPECT_EQ(limits.hihi, u"90");
  EXPECT_EQ(limits.hi, u"70");
  EXPECT_EQ(limits.lo, u"-10");
  EXPECT_EQ(limits.lolo, u"-25");
}

// The control dialog's condition row exists only when the target node carries
// a DataItemType_OutputCondition formula, and it reads "Satisfied" only when
// that formula evaluates truthy over live data. Both are fixture properties
// and both are invisible to the PNG comparison — the row's absence just makes
// the dialog shorter — so assert them here: without this the fixture could
// silently lose the property and ti-remote-control-enabled.png would go back
// to missing the row the docs original shows.
TEST_F(ScreenshotGenerator, ControlDialogNodeCarriesItsOutputCondition) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  const scada::NodeId node_id = FixtureConfig().dialog_analog_node_id;
  ASSERT_TRUE(scada::screenshot_generator::FetchNodesResident(
      executor_, app_.node_service(),
      std::span<const scada::NodeId>{&node_id, 1}));

  Profile profile;
  auto model = std::make_shared<WriteModel>(
      WriteContext{.executor_ = executor_,
                   .timed_data_service_ = app_.timed_data_service(),
                   .node_id_ = node_id,
                   .profile_ = profile,
                   .manual_ = false});

  EXPECT_TRUE(model->has_condition())
      << "the fixture node lost its DataItemType_OutputCondition; the control "
         "dialog would render without its condition row";

  // The condition subscribes through TimedDataService like any other item, so
  // its first value arrives on a later turn of the loop.
  scada::screenshot_generator::PumpEventLoopFor(std::chrono::milliseconds{500});

  EXPECT_TRUE(model->IsConditionOk())
      << "the condition formula must evaluate truthy over the fixture data, so "
         "the dialog reads Satisfied and its OK button is enabled";
}

// The mirror of the test above, for the capture that shows the dialog refusing
// the command. ti-remote-control-disabled.png is the same `write-remote` kind
// over a different node — the fixture cannot make one node's condition read
// both ways in a single run — so the whole capture rests on the dialog spec's
// `node` override reaching a node whose formula is falsy. Neither half is
// visible to check_screenshots.py, which only asserts the PNG exists: an
// override silently dropped, or a formula that quietly became true, would
// still produce a file, and it would be the enabled dialog under the disabled
// dialog's name.
TEST_F(ScreenshotGenerator, DisabledControlDialogNodeConditionIsUnsatisfied) {
  // Read the spec straight out of the fixture rather than from
  // FixtureConfig().dialogs: that vector is filtered by the managed-image gate
  // and by
  // --only, and the themed ctest run passes --only.
  const boost::json::value* dialog = nullptr;
  for (const auto& js : FixtureConfig().json.at("dialogs").as_array()) {
    if (js.at("filename").as_string() == "ti-remote-control-disabled.png")
      dialog = &js;
  }
  ASSERT_NE(dialog, nullptr)
      << "the fixture lost the disabled control-dialog capture";

  const auto* node = dialog->as_object().if_contains("node");
  ASSERT_NE(node, nullptr)
      << "the disabled variant needs its own node: sharing the fixture-wide "
         "dialog_analog_node_id renders the enabled dialog instead";
  const scada::NodeId node_id =
      NodeIdFromScadaString(std::string_view(node->as_string()));
  ASSERT_NE(node_id, FixtureConfig().dialog_analog_node_id);

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));
  ASSERT_TRUE(scada::screenshot_generator::FetchNodesResident(
      executor_, app_.node_service(),
      std::span<const scada::NodeId>{&node_id, 1}));

  const scada::NodeId enabled_node_id = FixtureConfig().dialog_analog_node_id;
  ASSERT_TRUE(scada::screenshot_generator::FetchNodesResident(
      executor_, app_.node_service(),
      std::span<const scada::NodeId>{&enabled_node_id, 1}));

  Profile profile;
  auto make_model = [&](const scada::NodeId& id) {
    return std::make_shared<WriteModel>(
        WriteContext{.executor_ = executor_,
                     .timed_data_service_ = app_.timed_data_service(),
                     .node_id_ = id,
                     .profile_ = profile,
                     .manual_ = false});
  };
  // Both nodes, in one run and over one pump. IsConditionOk() also reads false
  // before the first value lands, so the disabled expectation on its own would
  // pass on a condition that never evaluated at all — a subscription that
  // failed, a formula that did not parse, a pump too short to matter. The
  // enabled node is the control: it shares the service and the pump, so it
  // reads true only if conditions really are being evaluated in this run.
  auto model = make_model(node_id);
  auto enabled_model = make_model(enabled_node_id);

  EXPECT_TRUE(model->has_condition())
      << "without a DataItemType_OutputCondition the dialog renders no "
         "condition row at all, which is neither variant";

  scada::screenshot_generator::PumpEventLoopFor(std::chrono::milliseconds{500});

  EXPECT_TRUE(enabled_model->IsConditionOk())
      << "the control for this test: if the enabled node's condition does not "
         "read true either, conditions are not being evaluated at all and the "
         "expectation below proves nothing";
  EXPECT_FALSE(model->IsConditionOk())
      << "the condition formula must evaluate falsy over the fixture data, so "
         "the dialog reads Not satisfied and its OK button is disabled";
}

// The TS captures are the same two dialog kinds as the TI ones over a
// different node, so nothing in the spec says which of the two dialogs will be
// built: WriteModel decides from the node alone, taking its discrete branch
// from TimedDataSpec::logical() (a DiscreteItemType test) and its two state
// labels from the node's HasTsFormat target. Both halves are invisible to
// check_screenshots.py, which only asserts the PNG exists — a lost `node`
// override, a node that stopped being a DiscreteItemType, or a dropped
// HasTsFormat reference each still produce a file, and it would be the analog
// TI dialog, or the untranslated «On»/«Off» fallback, under a ts-* name. That
// is exactly how these three shipped before 2026-08-15.
TEST_F(ScreenshotGenerator, DiscreteControlDialogsRenderTheirTsFormatStates) {
  // Read the specs straight out of the fixture rather than from
  // FixtureConfig().dialogs: that vector is filtered by the managed-image gate
  // and by
  // --only, and the themed ctest run passes --only.
  struct DiscreteCapture {
    std::string_view filename;
    bool manual = false;
    // What the node's DataItemType_OutputCondition must evaluate to. The
    // enabled and disabled control captures are two nodes precisely because
    // one node cannot read both ways in a single run.
    bool condition_ok = false;
  };
  const DiscreteCapture kCaptures[] = {
      {.filename = "ts-manual-control.png", .manual = true},
      {.filename = "ts-remote-control-enabled.png", .condition_ok = true},
      {.filename = "ts-remote-control-disabled.png", .condition_ok = false},
  };

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  for (const auto& capture : kCaptures) {
    SCOPED_TRACE(capture.filename);

    const boost::json::value* dialog = nullptr;
    for (const auto& js : FixtureConfig().json.at("dialogs").as_array()) {
      if (js.at("filename").as_string() == capture.filename)
        dialog = &js;
    }
    ASSERT_NE(dialog, nullptr) << "the fixture lost this discrete capture";

    const auto* node = dialog->as_object().if_contains("node");
    ASSERT_NE(node, nullptr)
        << "a TS capture needs its own node: falling back to the fixture-wide "
           "dialog_analog_node_id renders the analog TI dialog under a TS name";
    const scada::NodeId node_id =
        NodeIdFromScadaString(std::string_view(node->as_string()));
    ASSERT_NE(node_id, FixtureConfig().dialog_analog_node_id);

    ASSERT_TRUE(scada::screenshot_generator::FetchNodesResident(
        executor_, app_.node_service(),
        std::span<const scada::NodeId>{&node_id, 1}));

    Profile profile;
    auto model = std::make_shared<WriteModel>(
        WriteContext{.executor_ = executor_,
                     .timed_data_service_ = app_.timed_data_service(),
                     .node_id_ = node_id,
                     .profile_ = profile,
                     .manual_ = capture.manual});

    ASSERT_TRUE(model->discrete())
        << "the node is not a DiscreteItemType, so the dialog offers an "
           "editable numeric field with engineering units — the TI dialog";

    // Both failure shapes matter and they look nothing alike. A HasTsFormat
    // reference that does not resolve at all leaves the built-in «On»/«Off»
    // fallback; one that resolves to a node whose label properties are not
    // resident reads back empty, and WriteModel has no per-label fallback, so
    // the combo renders two blank rows — a configured item looking
    // unconfigured. The second is what shipped before FetchNodesResident grew
    // its linked-reference wave.
    const auto states = model->GetDiscreteStates();
    ASSERT_EQ(states.size(), 2u);
    EXPECT_FALSE(states[0].empty())
        << "the open-state label read back empty: the HasTsFormat target's "
           "label properties are not resident";
    EXPECT_FALSE(states[1].empty())
        << "the close-state label read back empty: the HasTsFormat target's "
           "label properties are not resident";
    EXPECT_NE(states[0], states[1])
        << "both states carry the same label, so the operator cannot tell "
           "which one they are commanding";
    EXPECT_NE(states[0], DefaultOpenLabel())
        << "the open-state label fell back to the built-in default, so the "
           "node's HasTsFormat reference no longer resolves";
    EXPECT_NE(states[1], DefaultCloseLabel())
        << "the close-state label fell back to the built-in default, so the "
           "node's HasTsFormat reference no longer resolves";

    // The current value and the condition both arrive over
    // TimedDataService, so they land on a later turn of the loop.
    scada::screenshot_generator::PumpEventLoopFor(
        std::chrono::milliseconds{500});

    // base_value 0 in the fixture. The combo pre-selects the opposite state
    // (WriteModel inverts deliberately: a command proposes the other state),
    // so index 1 here means the value was delivered — an undelivered value
    // defaults to true and lands on index 0.
    EXPECT_EQ(model->GetCurrentDiscreteState(), 1)
        << "the fixture value did not reach the dialog, so the pre-selected "
           "state is the get_or default rather than the node's";

    if (capture.manual)
      continue;

    EXPECT_TRUE(model->has_condition())
        << "without a DataItemType_OutputCondition the dialog renders no "
           "condition row at all, which is neither control variant";
    EXPECT_EQ(model->IsConditionOk(), capture.condition_ok)
        << "the condition formula must evaluate " << capture.condition_ok
        << " over the fixture data for this capture to be the state it is "
           "named after";
  }
}

// A data group's Value column shows the link state of the device bound to it,
// and the binding is a fixture property no rendered image can be checked
// against: with no HasDevice reference `DataGroupVisibleNode::GetText()`
// returns the empty string, so every group row renders a blank cell and the
// capture still passes check_screenshots.py — which asserts existence,
// dimensions and pairwise distinctness, never text. That is how devices.png
// came to sit beside client.md's sentence «Для групп отображается состояние
// связи с устройством, привязанного к группе» while illustrating nothing.
//
// The state has to be read, not just bound: DeviceStateNotifier addresses the
// device's runtime components as MakeNestedNodeId(device, "Online"), an id no
// fixture node carries, and reaches the fixture's free-standing TS.xxxx
// variable only because GetMutableNestedNode decomposes a nested id back into
// a browse-name child walk (common/address_space/address_space_util.cpp). So
// the assertion is on the rendered text rather than on the reference: a
// resolution path that stopped working would leave the reference in place and
// the cell blank, and the PNG would still be perfectly valid.
TEST_F(ScreenshotGenerator, DataGroupShowsItsDeviceLinkState) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor_, app_.node_service()));

  // Read the bound groups out of the fixture rather than naming one here, so
  // renaming or re-binding the group cannot leave this test asserting a
  // binding nothing carries.
  std::vector<scada::NodeId> bound_groups;
  for (const auto& node : FixtureConfig().json.at("nodes").as_array()) {
    const auto& object = node.as_object();
    const auto* references = object.if_contains("references");
    if (!references)
      continue;
    for (const auto& reference : references->as_array()) {
      if (NodeIdFromScadaString(
              std::string_view(reference.as_object().at("type").as_string())) ==
          scada::data_items::id::HasDevice) {
        bound_groups.push_back(NodeIdFromScadaString(
            std::string_view(object.at("id").as_string())));
      }
    }
  }
  ASSERT_FALSE(bound_groups.empty())
      << "no fixture group carries a HasDevice reference, so the object "
         "tree's Value column is blank on every group row";

  ASSERT_TRUE(scada::screenshot_generator::FetchNodesResident(
      executor_, app_.node_service(), bound_groups));

  for (const scada::NodeId& group_id : bound_groups) {
    SCOPED_TRACE(NodeIdToScadaString(group_id));

    NodeRef group = app_.node_service().GetNode(group_id);
    ASSERT_TRUE(group);
    ASSERT_TRUE(IsInstanceOf(group, scada::data_items::id::DataGroupType))
        << "only a DataGroupType instance gets a DataGroupVisibleNode; on any "
           "other node the HasDevice reference is dead weight";

    DataGroupVisibleNode visible_node{app_.timed_data_service(), group};

    // The device's Online component is subscribed through TimedDataService, so
    // its first value lands on a later turn of the loop.
    scada::screenshot_generator::PumpEventLoopFor(
        std::chrono::milliseconds{500});

    EXPECT_EQ(visible_node.GetText(), ToLocalizedString(DeviceState::Online))
        << "the group reads «"
        << QString::fromStdU16String(visible_node.GetText()).toStdString()
        << "». An empty cell means the state never resolved — either the "
           "HasDevice target is unreachable, or its Online component is not "
           "at MakeNestedNodeId(device, \"Online\") where the notifier looks. "
           "The fixture binds the online device on purpose: hardware-tree.png "
           "is what shows the offline and disabled states.";
  }
}
