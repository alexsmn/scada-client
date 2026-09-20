// Assertions about the nodes the limits and control dialog captures target.
//
// A dialog built over a node missing its bands, its output condition or its
// TS format still lays out perfectly and still saves a PNG of exactly the
// spec's dimensions — the limits.png failure mode. These pin the node, so the
// fixture fails rather than the picture quietly emptying.

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
