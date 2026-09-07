#include "modules/write/write_model.h"

#include "base/async_completion.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "model/nested_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/static/static_node_service.h"
#include "profile/profile.h"
#include "scada/attribute_service_mock.h"
#include "scada/co_result.h"
#include "scada/method_service_mock.h"
#include "timed_data/timed_data_service_mock.h"

#include <gmock/gmock.h>

#include <optional>

using namespace testing;

namespace {

constexpr scada::NodeId kDataItemTypeId{9001, 1};
constexpr scada::NodeId kDataItemId{9002, 1};

// The per-item Control object carrying Select/Operate/Cancel. Addressed by a
// nested node id rather than by browsing, exactly as WriteModel builds it.
const scada::NodeId kControlNodeId =
    scada::MakeNestedNodeId(kDataItemId, scada::kControlObjectName);

class RecordingDialogService : public DialogService {
 public:
  explicit RecordingDialogService(AnyExecutor executor)
      : executor_{std::move(executor)} {}

  UiView* GetDialogOwningWindow() const override { return nullptr; }
  UiView* GetParentWidget() const override { return nullptr; }

  Awaitable<MessageBoxResult> RunMessageBox(std::u16string_view message,
                                            std::u16string_view title,
                                            MessageBoxMode mode) override {
    messages.emplace_back(message);
    titles.emplace_back(title);
    modes.emplace_back(mode);
    message_box_completion_ =
        std::make_unique<scada::base::AsyncCompletion>(executor_);
    co_await message_box_completion_->Wait();
    co_return message_box_result_;
  }

  Awaitable<std::filesystem::path> SelectOpenFile(
      std::u16string_view title) override {
    throw std::exception{};
    co_return std::filesystem::path{};
  }

  Awaitable<std::filesystem::path> SelectSaveFile(
      const SaveParams& params) override {
    throw std::exception{};
    co_return std::filesystem::path{};
  }

  void CompleteMessageBox(MessageBoxResult result = MessageBoxResult::Ok) {
    message_box_result_ = result;
    message_box_completion_->Complete();
  }

  std::vector<std::u16string> messages;
  std::vector<std::u16string> titles;
  std::vector<MessageBoxMode> modes;

 private:
  AnyExecutor executor_;
  MessageBoxResult message_box_result_ = MessageBoxResult::Ok;
  std::unique_ptr<scada::base::AsyncCompletion> message_box_completion_;
};

class WriteModelTest : public Test {
 protected:
  WriteModelTest()
      : node_service_{scada::services{.attribute_service = &attribute_service_,
                                      .method_service = &method_service_}},
        dialog_service_{executor_} {
    node_service_.Add(
        scada::NodeState{.node_id = kDataItemTypeId,
                         .node_class = scada::NodeClass::VariableType,
                         .attributes = {.display_name = u"Output type"}});

    ON_CALL(timed_data_service_, GetFormulaTimedData(_, _))
        .WillByDefault(Return(timed_data_));
    // Resolved lazily: the item is seeded by CreateModel(), after this
    // constructor runs, and a NodeRef taken before it exists is null — which
    // silently made every model read the OutputTwoStaged default.
    ON_CALL(*timed_data_, GetNode()).WillByDefault(Invoke([this] {
      return node_service_.GetNode(kDataItemId);
    }));
    ON_CALL(*timed_data_, GetTitle()).WillByDefault(Return(u"Output"));
  }

  // Seeds the data item and builds the model against it.
  //
  // The item is seeded here rather than in the constructor because
  // StaticNodeService::Add is try_emplace-based: adding a node id that already
  // exists is a SILENT no-op, so OutputTwoStaged cannot be changed after the
  // fact. The model reads it once, at construction.
  std::shared_ptr<WriteModel> CreateModel(bool two_staged = false) {
    node_service_.Add(
        scada::NodeState{.node_id = kDataItemId,
                         .node_class = scada::NodeClass::Variable,
                         .type_definition_id = kDataItemTypeId,
                         .attributes = {.display_name = u"Output"}}
            .set_property(scada::data_items::id::DataItemType_OutputTwoStaged,
                          two_staged)
            .set_property(scada::data_items::id::DataItemType_Locked, false));

    // The item's Control object, which carries Select/Operate/Cancel. It has
    // to exist here because StaticNodeService::GetScadaNode answers an unknown
    // id with a null `scada::node` — one whose services are all null, so every
    // Call would short-circuit to Bad_Disconnected without reaching the mock.
    // The production node service resolves this nested id as a cursor instead;
    // the configuration node manager never enumerates it as a child.
    node_service_.Add(scada::NodeState{
        .node_id = kControlNodeId,
        .node_class = scada::NodeClass::Object,
        .type_definition_id = scada::data_items::id::DataItemControlType,
        .attributes = {.display_name = u"Control"}});

    auto model = std::make_shared<WriteModel>(
        WriteContext{executor_, timed_data_service_, kDataItemId, profile_,
                     /*manual_=*/false});
    model->set_dialog_service(&dialog_service_);
    model->status_change_handler = [this] { ++status_changes_; };
    model->completion_handler = [this](bool ok) { completion_ = ok; };
    return model;
  }

  // Drives one write through to a refused completion and dismisses the error
  // box, so each case below asserts one consequence of the same run.
  void RunRefusedWrite(const std::shared_ptr<WriteModel>& model) {
    scada::base::AsyncCompletion completion{executor_};
    std::optional<scada::StatusOr<std::vector<scada::StatusCode>>> result;

    EXPECT_CALL(attribute_service_, Write(_, _))
        .WillOnce([&](scada::ServiceContext, std::vector<scada::WriteValue>)
                      -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
          co_await completion.Wait();
          co_return std::move(*result);
        });

    model->Write(7.0, /*lock=*/false);
    Drain(executor_);

    result = std::vector{scada::StatusCode::Bad};
    completion.Complete();
    Drain(executor_);

    // Absent when there is no dialog service to report through.
    if (!dialog_service_.modes.empty()) {
      dialog_service_.CompleteMessageBox();
      Drain(executor_);
    }
  }

  TestExecutor executor_;
  StrictMock<scada::MockAttributeService> attribute_service_;
  NiceMock<scada::MockMethodService> method_service_;
  NiceMock<MockTimedDataService> timed_data_service_;
  std::shared_ptr<NiceMock<MockTimedData>> timed_data_ =
      std::make_shared<NiceMock<MockTimedData>>();
  StaticNodeService node_service_;
  Profile profile_;
  RecordingDialogService dialog_service_;
  int status_changes_ = 0;
  std::optional<bool> completion_;
  // Methods invoked on the Control object, in order, with their arguments.
  std::vector<scada::NodeId> called_methods_;
  std::vector<std::vector<scada::Variant>> called_arguments_;
};

}  // namespace

TEST_F(WriteModelTest, SuccessfulWriteCompletesAfterAttributeCallback) {
  profile_.control_confirmation = false;
  scada::base::AsyncCompletion completion{executor_};
  std::optional<scada::StatusOr<std::vector<scada::StatusCode>>> result;

  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce(DoAll(
          WithArg<1>([](const auto& inputs) {
            ASSERT_EQ(inputs.size(), 1u);
            EXPECT_EQ(inputs[0].node_id, kDataItemId);
            EXPECT_EQ(inputs[0].attribute_id, scada::AttributeId::Value);
            EXPECT_DOUBLE_EQ(inputs[0].value.template get<double>(), 42.0);
            EXPECT_FALSE(inputs[0].flags.select());
          }),
          Invoke([&](scada::ServiceContext, std::vector<scada::WriteValue>)
                     -> Awaitable<
                         scada::StatusOr<std::vector<scada::StatusCode>>> {
            co_await completion.Wait();
            co_return std::move(*result);
          })));

  auto model = CreateModel();
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  EXPECT_FALSE(completion_.has_value());

  result = std::vector{scada::StatusCode::Good};
  completion.Complete();
  Drain(executor_);

  EXPECT_EQ(completion_, true);
  EXPECT_EQ(status_changes_, 1);
  EXPECT_TRUE(dialog_service_.modes.empty());
}

// A two-staged control is a select, then the operator's confirmation, then an
// operate — three distinct steps, where it used to be two indistinguishable
// value writes. The confirmation sits between a real select and a real
// operate, so nothing reaches the equipment until it is answered.
TEST_F(WriteModelTest, TwoStagedControlSelectsConfirmsThenOperates) {
  EXPECT_CALL(method_service_, Call(kControlNodeId, _, _, _))
      .WillRepeatedly([this](scada::NodeId, scada::NodeId method_id,
                             std::vector<scada::Variant> arguments,
                             scada::ServiceContext) {
        called_methods_.push_back(method_id);
        called_arguments_.push_back(std::move(arguments));
        return scada::MakeMethodCallResult();
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  // Phase one: the select has gone out, carrying the commanded value, and the
  // operator is being asked to confirm. The operate must NOT have been sent.
  EXPECT_THAT(called_methods_,
              ElementsAre(scada::data_items::id::DataItemControlType_Select));
  ASSERT_EQ(called_arguments_.size(), 1u);
  ASSERT_EQ(called_arguments_[0].size(), 1u);
  EXPECT_DOUBLE_EQ(called_arguments_[0][0].get<double>(), 42.0);
  ASSERT_THAT(dialog_service_.modes,
              ElementsAre(MessageBoxMode::QuestionYesNoDefaultNo));
  EXPECT_FALSE(completion_.has_value());

  // The prompt tells the operator the device is armed and waiting, which is
  // only true because a select really did precede it.
  EXPECT_NE(dialog_service_.messages.at(0).find(u"ready to execute"),
            std::u16string::npos);

  dialog_service_.CompleteMessageBox(MessageBoxResult::Yes);
  Drain(executor_);

  // Phase two: confirming operates on the selection already held.
  EXPECT_THAT(called_methods_,
              ElementsAre(scada::data_items::id::DataItemControlType_Select,
                          scada::data_items::id::DataItemControlType_Operate));
  ASSERT_EQ(called_arguments_.size(), 2u);
  ASSERT_EQ(called_arguments_[1].size(), 1u);
  EXPECT_DOUBLE_EQ(called_arguments_[1][0].get<double>(), 42.0);
  EXPECT_EQ(completion_, true);
}

// Declining releases the selection instead of leaving the outstation armed
// until its own sboTimeout. The cancel is fire-and-forget: no IEC 60870 driver
// implements one, so Bad_NotSupported is the expected answer and must not
// raise an error dialog at an operator who has already moved on.
TEST_F(WriteModelTest, DecliningTwoStagedControlCancelsTheSelection) {
  EXPECT_CALL(method_service_, Call(kControlNodeId, _, _, _))
      .WillRepeatedly([this](scada::NodeId, scada::NodeId method_id,
                             std::vector<scada::Variant>,
                             scada::ServiceContext) {
        called_methods_.push_back(method_id);
        // Answer the cancel the way a real driver does.
        return scada::MakeMethodCallResult(
            method_id == scada::data_items::id::DataItemControlType_Cancel
                ? scada::Status{scada::StatusCode::Bad_NotSupported}
                : scada::Status{scada::StatusCode::Good});
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  ASSERT_THAT(called_methods_,
              ElementsAre(scada::data_items::id::DataItemControlType_Select));
  ASSERT_THAT(dialog_service_.modes,
              ElementsAre(MessageBoxMode::QuestionYesNoDefaultNo));

  dialog_service_.CompleteMessageBox(MessageBoxResult::No);
  Drain(executor_);

  // The selection is released and the operate is never sent.
  EXPECT_THAT(called_methods_,
              ElementsAre(scada::data_items::id::DataItemControlType_Select,
                          scada::data_items::id::DataItemControlType_Cancel));
  EXPECT_EQ(completion_, false);

  // The rejected cancel is swallowed: still just the one confirmation prompt,
  // no error box behind it.
  EXPECT_THAT(dialog_service_.modes,
              ElementsAre(MessageBoxMode::QuestionYesNoDefaultNo));
}

TEST_F(WriteModelTest, ControlCommandConfirmationReviewsPresentAndCommand) {
  // control_confirmation defaults to true, so a control write must prompt for
  // a deliberate review before anything reaches the device.
  auto model = CreateModel();
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  // A yes/no confirmation is shown and, crucially, nothing is written yet —
  // StrictMock<MockAttributeService> would fail on an unexpected Write().
  ASSERT_THAT(dialog_service_.modes,
              ElementsAre(MessageBoxMode::QuestionYesNoDefaultNo));
  EXPECT_FALSE(completion_.has_value());

  // The prompt reviews the present reading vs. the commanded value and warns
  // that the action is irreversible — not a bare "Switch X to Y?".
  const std::u16string& message = dialog_service_.messages.at(0);
  EXPECT_NE(message.find(u"Present"), std::u16string::npos);
  EXPECT_NE(message.find(u"Command"), std::u16string::npos);
  EXPECT_NE(message.find(u"42"), std::u16string::npos);
  EXPECT_NE(message.find(u"cannot be undone remotely"), std::u16string::npos);
}

TEST_F(WriteModelTest, RefusedWriteIsReportedToTheOperator) {
  profile_.control_confirmation = false;
  auto model = CreateModel();

  RunRefusedWrite(model);

  EXPECT_THAT(dialog_service_.modes, ElementsAre(MessageBoxMode::Error));
}

// The half an operator notices, and task 519: a refused command must leave the
// dialog open on the value they chose. `completion_handler(false)` is what
// keeps it there — completing with true closed it the moment the error box was
// dismissed, so retrying meant reopening Control and re-entering the value.
// LimitModelTest.RefusedWriteLeavesTheDialogOpen is the same assertion on the
// side where this shape was settled first.
TEST_F(WriteModelTest, RefusedWriteLeavesTheDialogOpen) {
  profile_.control_confirmation = false;
  auto model = CreateModel();

  RunRefusedWrite(model);

  ASSERT_TRUE(completion_.has_value());
  EXPECT_FALSE(*completion_);
}

// The dialog stays open now, so the status line is visible after the failure —
// and `writing_` left set made GetStatusText() go on saying "Controlling..."
// over a command that had already been refused.
TEST_F(WriteModelTest, RefusedWriteClearsTheStatusText) {
  profile_.control_confirmation = false;
  auto model = CreateModel();

  RunRefusedWrite(model);

  EXPECT_TRUE(model->GetStatusText().empty());
}

// The error is not reported when there is nothing to report through, but the
// dialog must still be released rather than stranded with a dead Control
// button — and the failure path must not dereference the absent service.
TEST_F(WriteModelTest, RefusedWriteWithoutADialogServiceStillCompletes) {
  profile_.control_confirmation = false;
  auto model = CreateModel();
  model->set_dialog_service(nullptr);

  RunRefusedWrite(model);

  EXPECT_THAT(dialog_service_.modes, IsEmpty());
  ASSERT_TRUE(completion_.has_value());
  EXPECT_FALSE(*completion_);
}

// A written command still closes the dialog and reports nothing. Without this
// the fix above could "pass" by never completing at all, which would strand the
// operator on a dialog whose Control button no longer does anything.
TEST_F(WriteModelTest, SuccessfulWriteStillClosesTheDialog) {
  profile_.control_confirmation = false;
  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce([&](scada::ServiceContext, std::vector<scada::WriteValue>)
                    -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
        co_return std::vector{scada::StatusCode::Good};
      });

  auto model = CreateModel();
  model->Write(7.0, /*lock=*/false);
  Drain(executor_);

  ASSERT_TRUE(completion_.has_value());
  EXPECT_TRUE(*completion_);
  EXPECT_THAT(dialog_service_.modes, IsEmpty());
}

TEST_F(WriteModelTest, DestroyedModelDropsPendingWriteCompletion) {
  profile_.control_confirmation = false;
  scada::base::AsyncCompletion completion{executor_};
  std::optional<scada::StatusOr<std::vector<scada::StatusCode>>> result;

  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce([&](scada::ServiceContext, std::vector<scada::WriteValue>)
                    -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
        co_await completion.Wait();
        co_return std::move(*result);
      });

  auto model = CreateModel();
  model->Write(9.0, /*lock=*/false);
  Drain(executor_);

  model.reset();
  result = std::vector{scada::StatusCode::Good};
  completion.Complete();
  Drain(executor_);

  EXPECT_FALSE(completion_.has_value());
  EXPECT_TRUE(dialog_service_.modes.empty());
}

// Regression: WriteDialog::accept used to call RunMessageBox directly and
// discard the returned awaitable. It is lazy, so the coroutine never started,
// no box was ever shown, and an unparseable entry left the dialog refusing to
// close with nothing on screen to explain why. Asserting on the recorded box
// is what proves the awaitable actually ran — the pre-fix code records
// nothing here.
TEST_F(WriteModelTest, ReportedInputErrorReachesTheOperator) {
  auto model = CreateModel();

  model->ReportInputError(u"Incorrect floating point value.");
  Drain(executor_);

  ASSERT_THAT(dialog_service_.modes, ElementsAre(MessageBoxMode::Error));
  EXPECT_EQ(dialog_service_.messages.at(0), u"Incorrect floating point value.");
  // The window title, so the box is attributable to this dialog rather than
  // arriving bare.
  EXPECT_EQ(dialog_service_.titles.at(0), model->GetWindowTitle());
}

// Rejected input is not a completed write: nothing is sent to the point and
// the dialog must stay open, which is what completion_handler(true) would
// close. ReportWriteErrorAsync deliberately does complete — this path must
// not share that behaviour.
TEST_F(WriteModelTest, ReportedInputErrorNeitherWritesNorCompletes) {
  auto model = CreateModel();

  // attribute_service_ is a StrictMock, so any Write would fail the test.
  model->ReportInputError(u"Incorrect floating point value.");
  Drain(executor_);
  dialog_service_.CompleteMessageBox();
  Drain(executor_);

  EXPECT_FALSE(completion_.has_value());
}
