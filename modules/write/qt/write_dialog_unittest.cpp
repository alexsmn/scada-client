#include "modules/write/write_dialog.h"

#include "aui/dialog_service.h"
#include "aui/qt/dialog_test_util.h"
#include "aui/test/app_environment.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "node_service/static/static_node_service.h"
#include "profile/profile.h"
#include "timed_data/timed_data_service_mock.h"

#include <QComboBox>
#include <QDialog>
#include <QEvent>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <filesystem>

using namespace testing;

namespace {

constexpr scada::NodeId kDataItemId{9002, 1};

class TestDialogService : public DialogService {
 public:
  UiView* GetDialogOwningWindow() const override { return nullptr; }
  UiView* GetParentWidget() const override { return nullptr; }

  Awaitable<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                            std::u16string_view title,
                                            MessageBoxMode mode) override {
    throw std::exception{};
  }

  Awaitable<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override {
    throw std::exception{};
  }

  Awaitable<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override {
    throw std::exception{};
  }
};

// Drives the real Control dialog over a discrete item whose current value has
// not arrived yet — the state every such dialog starts in, because the reading
// comes over TimedDataService a turn or more after the dialog is constructed.
//
// `manual_ = true` deliberately: it takes the item's OutputCondition and its
// Control object out of the picture, neither of which this fixture is about.
class WriteDialogSeedTest : public Test {
 protected:
  WriteDialogSeedTest() {
    ON_CALL(timed_data_service_, GetFormulaTimedData(_, _))
        .WillByDefault(Return(timed_data_));
    ON_CALL(*timed_data_, GetNode()).WillByDefault(Invoke([this] {
      return node_service_.GetNode(kDataItemId);
    }));
    ON_CALL(*timed_data_, GetTitle()).WillByDefault(Return(u"Output"));

    // The type node has to be resident too, not just named as the item's
    // type_definition_id: IsInstanceOf walks the supertype chain from the
    // resolved NodeRef, and an unresolvable type definition reads as "not a
    // discrete item" rather than as an error — which silently puts the dialog
    // on its analog branch, with an editable box and no state labels at all.
    node_service_.Add(
        scada::NodeState{.node_id = scada::data_items::id::DiscreteItemType,
                         .node_class = scada::NodeClass::VariableType,
                         .attributes = {.display_name = u"Discrete item"}});

    // DiscreteItemType is what WriteModel reads to take its discrete branch
    // (TimedDataSpec::logical), which is the branch that renders a combo of
    // state labels rather than an editable value box.
    node_service_.Add(scada::NodeState{
        .node_id = kDataItemId,
        .node_class = scada::NodeClass::Variable,
        .type_definition_id = scada::data_items::id::DiscreteItemType,
        .attributes = {.display_name = u"Output"}});
  }

  std::shared_ptr<scada::aui::qt::test::AwaitableResult<void>> ShowDialog() {
    return scada::aui::qt::test::StartAwaitable(ExecuteWriteDialog(
        dialog_service_,
        WriteContext{executor_, timed_data_service_, kDataItemId, profile_,
                     /*manual_=*/true}));
  }

  // StartOwnedModalDialog destroys the dialog through deleteLater(), which
  // needs one more turn of the event loop than ProcessEventsUntilSettled
  // takes — it stops the instant the awaitable completes, leaving the deferred
  // delete queued. Skipping this leaves the dialog, its WriteModel and the
  // mock that model holds alive past the test, which gmock reports at exit as
  // a leaked mock and which fails the binary.
  ~WriteDialogSeedTest() override {
    QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    QApplication::processEvents();
  }

  // Delivers the item's current reading, the way a subscription would.
  void DeliverCurrentValue(bool value) {
    timed_data_->UpdateData(scada::DataValue{scada::Variant{value},
                                             scada::Qualifier{}, scada::Time{},
                                             scada::Time{}});
  }

  AppEnvironment app_env_;
  AnyExecutor executor_ = MakeAnyExecutor(std::make_shared<MessageLoopQt>());
  NiceMock<MockTimedDataService> timed_data_service_;
  std::shared_ptr<NiceMock<MockTimedData>> timed_data_ =
      std::make_shared<NiceMock<MockTimedData>>();
  StaticNodeService node_service_;
  Profile profile_;
  TestDialogService dialog_service_;
};

}  // namespace

// Regression for the defect this fixture was written for: the dialog seeded its
// value control once, in the constructor, and afterwards refreshed only the
// "Current value:" label. With no reading yet,
// `WriteModel::GetCurrentDiscreteState` falls back to `get_or(true)`, so the
// combo proposed the state the item turned out to be in already — beside a
// label that had since corrected itself to the other one.
TEST_F(WriteDialogSeedTest, ValueArrivingAfterConstructionReseedsTheProposal) {
  auto result = ShowDialog();

  scada::aui::qt::test::ProcessEventsUntilSettled(
      result, [this](QDialog& dialog) {
        auto* combo = dialog.findChild<QComboBox*>("valueComboBox");
        ASSERT_NE(combo, nullptr);

        // No reading yet: get_or(true) proposes index 0.
        EXPECT_EQ(combo->currentIndex(), 0);

        // The item reads false, so the dialog should now propose its opposite.
        DeliverCurrentValue(false);
        EXPECT_EQ(combo->currentIndex(), 1);

        dialog.reject();
      });
}

// The other half of the same rule, and the reason the re-seed is conditional: a
// value ticking in must never silently replace a selection the operator made
// deliberately. Clobbering that would be a worse failure than the stale
// proposal, since the operator is looking at a command they did not choose.
TEST_F(WriteDialogSeedTest, ValueArrivingAfterOperatorInputLeavesItAlone) {
  auto result = ShowDialog();

  scada::aui::qt::test::ProcessEventsUntilSettled(
      result, [this](QDialog& dialog) {
        auto* combo = dialog.findChild<QComboBox*>("valueComboBox");
        ASSERT_NE(combo, nullptr);

        // `activated` is what the operator's own selection emits;
        // setCurrentIndex alone is what the dialog does to itself.
        combo->setCurrentIndex(0);
        Q_EMIT combo->activated(0);

        DeliverCurrentValue(false);
        EXPECT_EQ(combo->currentIndex(), 0);

        dialog.reject();
      });
}
