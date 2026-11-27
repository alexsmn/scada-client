#include "modules/write/write_model.h"

#include "base/async_completion.h"
#include "base/format_time.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "base/utf_convert.h"
#include "common/formula_util.h"
#include "common/node_state.h"
#include "events/local_events.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/namespaces.h"
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

// A Call answer that arrives only once `answer` completes, and refuses.
// `answer` is a test-scope object, so the reference outlives the lazy frame.
scada::CoStatusOr<scada::CallResult> AwaitRefusal(
    scada::base::AsyncCompletion& answer) {
  co_await answer.Wait();
  co_return scada::MakeCallResult(scada::Status{scada::StatusCode::Bad});
}

// A Write answer that arrives only once `answer` completes, with `code` for
// the one value written.
scada::CoStatusOr<std::vector<scada::StatusCode>> AwaitWriteAnswer(
    scada::base::AsyncCompletion& answer,
    scada::StatusCode code) {
  co_await answer.Wait();
  co_return std::vector{code};
}

// A Call answer that arrives only once `answer` completes, and accepts.
scada::CoStatusOr<scada::CallResult> AwaitAcceptance(
    scada::base::AsyncCompletion& answer) {
  co_await answer.Wait();
  co_return scada::MakeCallResult(scada::Status{scada::StatusCode::Good});
}

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

// Stands in for the Qt review dialog: records every review the model asks
// for and answers it when the test says so.
class RecordingReviewer {
 public:
  explicit RecordingReviewer(AnyExecutor executor)
      : executor_{std::move(executor)} {}

  // What WriteModel installs through set_control_reviewer. A plain lambda
  // returning a coroutine that takes the review by value, as the real one is.
  WriteModel::ControlReviewer AsReviewer() {
    return [this](ControlReview review) { return Review(std::move(review)); };
  }

  void Complete(bool send) {
    send_ = send;
    completion_->Complete();
  }

  std::vector<ControlReview> reviews;

 private:
  Awaitable<bool> Review(ControlReview review) {
    reviews.push_back(std::move(review));
    completion_ = std::make_unique<scada::base::AsyncCompletion>(executor_);
    co_await completion_->Wait();
    co_return send_;
  }

  AnyExecutor executor_;
  bool send_ = false;
  std::unique_ptr<scada::base::AsyncCompletion> completion_;
};

class WriteModelTest : public Test {
 protected:
  WriteModelTest()
      : node_service_{scada::services{.attribute_service = &attribute_service_,
                                      .method_service = &method_service_}},
        dialog_service_{executor_},
        reviewer_{executor_} {
    node_service_.Add(
        scada::NodeState{.node_id = kDataItemTypeId,
                         .node_class = scada::NodeClass::VariableType,
                         .attributes = {.display_name = u"Output type"}});

    ON_CALL(timed_data_service_, GetFormulaTimedData(_, _))
        .WillByDefault(Return(timed_data_));
    // Resolved lazily: the item is seeded by CreateModel(), after this
    // constructor runs, and a NodeRef taken before it exists is null — which
    // silently made every model read the OutputTwoStaged default.
    ON_CALL(*timed_data_, GetNode()).WillByDefault([this] {
      return node_service_.GetNode(kDataItemId);
    });
    ON_CALL(*timed_data_, GetTitle()).WillByDefault(Return(u"Output"));
  }

  // Seeds the data item and builds the model against it.
  //
  // The item is seeded here rather than in the constructor because
  // StaticNodeService::Add is try_emplace-based: adding a node id that already
  // exists is a SILENT no-op, so OutputTwoStaged cannot be changed after the
  // fact. The model reads it once, at construction.
  //
  // `output_formula` binds the item's Output channel, which decides the
  // device its command is routed to; empty leaves the device unknown.
  std::shared_ptr<WriteModel> CreateModel(bool two_staged = false,
                                          LocalEvents* local_events = nullptr,
                                          std::string output_formula = {}) {
    node_service_.Add(
        scada::NodeState{.node_id = kDataItemId,
                         .node_class = scada::NodeClass::Variable,
                         .type_definition_id = kDataItemTypeId,
                         .parent_id = parent_id_,
                         .reference_type_id = parent_id_.is_null()
                                                  ? scada::NodeId{}
                                                  : scada::id::Organizes,
                         .attributes = {.display_name = u"Output"}}
            .set_property(scada::data_items::id::DataItemType_OutputTwoStaged,
                          two_staged)
            .set_property(scada::data_items::id::DataItemType_Locked, false)
            .set_property(scada::data_items::id::DataItemType_Output,
                          scada::String{output_formula})
            .set_property(scada::data_items::id::DataItemType_OutputCondition,
                          scada::String{output_condition_}));

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

    auto model = std::make_shared<WriteModel>(WriteContext{
        .executor_ = executor_,
        .timed_data_service_ = timed_data_service_,
        .node_id_ = kDataItemId,
        .profile_ = profile_,
        .manual_ = false,
        .local_events_ = local_events ? local_events : &local_events_,
        .may_skip_confirmation_ = may_skip_confirmation_,
        .user_id_ = user_id_});
    model->set_dialog_service(&dialog_service_);
    model->set_control_reviewer(reviewer_.AsReviewer());
    model->status_change_handler = [this] { ++status_changes_; };
    model->completion_handler = [this](bool ok) { completion_ = ok; };
    return model;
  }

  // Seeds a device of `type_id` (with the supertype link `IsInstanceOf`
  // walks) and answers the Output formula of channel `nested` under it.
  std::string AddDeviceChannel(const scada::NodeId& type_id,
                               std::string_view nested = "1") {
    node_service_.Add(
        scada::NodeState{.node_id = scada::devices::id::DeviceType,
                         .node_class = scada::NodeClass::ObjectType});
    node_service_.Add(
        scada::NodeState{.node_id = type_id,
                         .node_class = scada::NodeClass::ObjectType,
                         .supertype_id = scada::devices::id::DeviceType});
    const scada::NodeId device_id{9100, 2};
    node_service_.Add(scada::NodeState{.node_id = device_id,
                                       .node_class = scada::NodeClass::Object,
                                       .type_definition_id = type_id});
    return MakeNodeIdFormula(scada::MakeNestedNodeId(device_id, nested));
  }

  // Answers the Control object's Select as unknown, as a 2.5.7 Server does,
  // and the flagged select Good once `select_answer` completes.
  void ExpectLegacySelect(scada::base::AsyncCompletion& select_answer) {
    ON_CALL(method_service_, Call)
        .WillByDefault([](scada::NodeId, scada::NodeId,
                          std::vector<scada::Variant>, scada::ServiceContext) {
          return scada::MakeMethodCallResult(
              scada::Status{scada::StatusCode::Bad_WrongNodeId});
        });
    EXPECT_CALL(attribute_service_, Write(_, _))
        .WillOnce([&select_answer](scada::ServiceContext,
                                   std::vector<scada::WriteValue>) {
          return AwaitWriteAnswer(select_answer, scada::StatusCode::Good);
        });
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
  LocalEvents local_events_;
  RecordingDialogService dialog_service_;
  RecordingReviewer reviewer_;
  // The account's right to switch confirmation off. True by default so the
  // cases that set `profile_.control_confirmation = false` to take the review
  // out of the picture still can; the gate's own cases clear it.
  bool may_skip_confirmation_ = true;
  // The signed-in account's user node; null is an anonymous session.
  scada::NodeId user_id_;
  // The item's OutputCondition formula, the interlock; empty for none.
  std::string output_condition_;
  // The group the item is organized under; null leaves it at the top.
  scada::NodeId parent_id_;
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
          [&](scada::ServiceContext, std::vector<scada::WriteValue>)
              -> Awaitable<scada::StatusOr<std::vector<scada::StatusCode>>> {
            co_await completion.Wait();
            co_return std::move(*result);
          }));

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

// A Variable of a third-party server carries no OutputTwoStaged property, and
// the property's default is Yes — which sent a Select to a Control object the
// server has never had, failing before the value was written. On a foreign
// server the value is written directly, the one command every OPC UA server
// understands (backlog 1054).
TEST_F(WriteModelTest, AForeignServersVariableIsWrittenWithoutASelect) {
  profile_.control_confirmation = false;
  node_service_.Add(scada::NodeState{
      .node_id = scada::NodeId{63},
      .node_class = scada::NodeClass::VariableType,
      .attributes = {.display_name = u"BaseDataVariableType"}});
  node_service_.Add(
      scada::NodeState{.node_id = kDataItemId,
                       .node_class = scada::NodeClass::Variable,
                       // BaseDataVariableType, ns=0;i=63: an ordinary
                       // third-party variable.
                       .type_definition_id = scada::NodeId{63},
                       .attributes = {.display_name = u"Demo.Setpoint"}});
  EXPECT_CALL(method_service_, Call).Times(0);
  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce(DoAll(WithArg<1>([](const auto& inputs) {
                        ASSERT_EQ(inputs.size(), 1u);
                        EXPECT_EQ(inputs[0].node_id, kDataItemId);
                        EXPECT_FALSE(inputs[0].flags.select());
                      }),
                      [](scada::ServiceContext, std::vector<scada::WriteValue>)
                          -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
                        co_return std::vector{scada::StatusCode::Good};
                      }));

  auto model = std::make_shared<WriteModel>(
      WriteContext{.executor_ = executor_,
                   .timed_data_service_ = timed_data_service_,
                   .node_id_ = kDataItemId,
                   .profile_ = profile_,
                   .manual_ = false,
                   .local_events_ = &local_events_,
                   .scada_server_ = false,
                   .may_skip_confirmation_ = may_skip_confirmation_});
  model->set_dialog_service(&dialog_service_);
  model->status_change_handler = [this] { ++status_changes_; };
  model->completion_handler = [this](bool ok) { completion_ = ok; };
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  EXPECT_EQ(completion_, true);
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
  ASSERT_THAT(reviewer_.reviews, SizeIs(1));
  EXPECT_TRUE(dialog_service_.modes.empty());
  EXPECT_FALSE(completion_.has_value());

  // The review tells the operator this is the operate stage and the device is
  // armed and waiting, which is only true because a select really did precede
  // it.
  EXPECT_NE(reviewer_.reviews.at(0).stage.find(u"Stage 2 of 2"),
            std::u16string::npos);
  EXPECT_NE(reviewer_.reviews.at(0).stage.find(u"waiting"),
            std::u16string::npos);

  reviewer_.Complete(/*send=*/true);
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
// until its own sboTimeout. A cancel the device refuses must not raise an
// error dialog at an operator who has already moved on — but it is reported as
// a local event, since the selection may still be armed (backlog 959).
TEST_F(WriteModelTest, DecliningTwoStagedControlCancelsTheSelection) {
  // A failure report must not depend on the success-report setting.
  profile_.show_write_ok = false;
  EXPECT_CALL(method_service_, Call(kControlNodeId, _, _, _))
      .WillRepeatedly([this](scada::NodeId, scada::NodeId method_id,
                             std::vector<scada::Variant>,
                             scada::ServiceContext) {
        called_methods_.push_back(method_id);
        // The device refuses the release (an IEC 104 negative confirmation).
        return scada::MakeMethodCallResult(
            method_id == scada::data_items::id::DataItemControlType_Cancel
                ? scada::Status{scada::StatusCode::Bad}
                : scada::Status{scada::StatusCode::Good});
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  ASSERT_THAT(called_methods_,
              ElementsAre(scada::data_items::id::DataItemControlType_Select));
  ASSERT_THAT(reviewer_.reviews, SizeIs(1));

  reviewer_.Complete(/*send=*/false);
  Drain(executor_);

  // The selection is released and the operate is never sent.
  EXPECT_THAT(called_methods_,
              ElementsAre(scada::data_items::id::DataItemControlType_Select,
                          scada::data_items::id::DataItemControlType_Cancel));
  EXPECT_EQ(completion_, false);

  // No error box behind the one review...
  EXPECT_TRUE(dialog_service_.modes.empty());

  // ...but the refusal is reported, naming the point, even though the
  // "report successful control" setting is off.
  ASSERT_THAT(local_events_.events(), SizeIs(1));
  EXPECT_EQ(local_events_.events()[0]->severity, scada::kSeverityCritical);
  EXPECT_NE(local_events_.events()[0]->message.text.find(u"Output"),
            std::u16string::npos);
}

// A released selection is the expected outcome, and Bad_NotSupported is a
// driver with no Cancel, which never armed one. Neither leaves anything for
// the operator to act on.
TEST_F(WriteModelTest, ReleasedOrNeverArmedSelectionReportsNothing) {
  for (scada::StatusCode cancel_answer :
       {scada::StatusCode::Good, scada::StatusCode::Bad_NotSupported}) {
    ON_CALL(method_service_, Call)
        .WillByDefault([cancel_answer](scada::NodeId, scada::NodeId method_id,
                                       std::vector<scada::Variant>,
                                       scada::ServiceContext) {
          return scada::MakeMethodCallResult(
              method_id == scada::data_items::id::DataItemControlType_Cancel
                  ? scada::Status{cancel_answer}
                  : scada::Status{scada::StatusCode::Good});
        });

    auto model = CreateModel(/*two_staged=*/true);
    model->Write(42.0, /*lock=*/false);
    Drain(executor_);
    reviewer_.Complete(/*send=*/false);
    Drain(executor_);

    EXPECT_EQ(completion_, false);
    EXPECT_THAT(local_events_.events(), IsEmpty());
  }
}

// The Server refuses the select of an object whose device cannot select
// (IEC 60870-5 commands other than a single command, every MODBUS and OPC
// object, IEC 61850 direct control) with Bad_NotSupported. The operator is told
// so in words, nothing is confirmed, operated or cancelled, and the dialog
// stays open (backlog 983). It used to answer Good, and the prompt said the
// device was ready to execute.
TEST_F(WriteModelTest, RefusedSelectExplainsAndArmsNothing) {
  EXPECT_CALL(method_service_, Call(kControlNodeId, _, _, _))
      .WillRepeatedly([this](scada::NodeId, scada::NodeId method_id,
                             std::vector<scada::Variant>,
                             scada::ServiceContext) {
        called_methods_.push_back(method_id);
        return scada::MakeMethodCallResult(
            scada::Status{scada::StatusCode::Bad_NotSupported});
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  // One error box, not the confirmation prompt.
  ASSERT_THAT(dialog_service_.modes, ElementsAre(MessageBoxMode::Error));
  const std::u16string& message = dialog_service_.messages.at(0);
  EXPECT_NE(message.find(u"Two-stage control is not available for this "
                         u"object"),
            std::u16string::npos);
  EXPECT_NE(message.find(u"Nothing was sent to the device."),
            std::u16string::npos);
  EXPECT_EQ(message.find(u"ready to execute"), std::u16string::npos);
  // Two-stage control is configured per object, which is the project
  // engineer's job, not the administrator's.
  EXPECT_NE(message.find(u"engineer who configures the project"),
            std::u16string::npos);
  EXPECT_EQ(message.find(u"administrator"), std::u16string::npos);
  EXPECT_TRUE(model->GetStatusText().empty());

  dialog_service_.CompleteMessageBox();
  Drain(executor_);

  EXPECT_THAT(called_methods_,
              ElementsAre(scada::data_items::id::DataItemControlType_Select));
  EXPECT_EQ(completion_, false);
  EXPECT_THAT(local_events_.events(), IsEmpty());

  // Closing the dialog sends no Cancel: there is no selection to release.
  model.reset();
  Drain(executor_);
  EXPECT_THAT(called_methods_,
              ElementsAre(scada::data_items::id::DataItemControlType_Select));
}

// A 2.5.7 Server has no per-item Control object: it refuses the Select as an
// unknown node before anything reaches the device. The 2.6 Client must then
// send the two stages the way the 2.5 Client did -- a value write flagged as a
// select, the confirmation, then a plain value write -- rather than telling the
// operator the device cannot select.
TEST_F(WriteModelTest, ServerWithoutControlObjectSelectsByFlaggedWrite) {
  EXPECT_CALL(method_service_, Call(kControlNodeId, _, _, _))
      .WillRepeatedly([this](scada::NodeId, scada::NodeId method_id,
                             std::vector<scada::Variant>,
                             scada::ServiceContext) {
        called_methods_.push_back(method_id);
        return scada::MakeMethodCallResult(
            scada::Status{scada::StatusCode::Bad_WrongNodeId});
      });
  std::vector<scada::WriteValue> writes;
  EXPECT_CALL(attribute_service_, Write(_, _))
      .Times(2)
      .WillRepeatedly([&writes](scada::ServiceContext,
                                std::vector<scada::WriteValue> inputs) {
        writes.insert(writes.end(), inputs.begin(), inputs.end());
        return scada::MakeAttributeResult<scada::StatusCode>(
            std::vector{scada::StatusCode::Good});
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  // The select went out a second time, as 2.5 sent it, and the operator is
  // asked to confirm -- not shown an error.
  ASSERT_EQ(writes.size(), 1u);
  EXPECT_EQ(writes[0].node_id, kDataItemId);
  EXPECT_TRUE(writes[0].flags.select());
  EXPECT_DOUBLE_EQ(writes[0].value.get<double>(), 42.0);
  ASSERT_THAT(reviewer_.reviews, SizeIs(1));
  EXPECT_TRUE(dialog_service_.modes.empty());
  EXPECT_NE(reviewer_.reviews.at(0).stage.find(u"waiting"),
            std::u16string::npos);

  reviewer_.Complete(/*send=*/true);
  Drain(executor_);

  // The operate is the unflagged write; no Operate call was sent to a Server
  // that has already said it has no Control object.
  ASSERT_EQ(writes.size(), 2u);
  EXPECT_FALSE(writes[1].flags.select());
  EXPECT_DOUBLE_EQ(writes[1].value.get<double>(), 42.0);
  EXPECT_THAT(called_methods_,
              ElementsAre(scada::data_items::id::DataItemControlType_Select));
  EXPECT_EQ(completion_, true);
}

// Declining after a 2.5-style select sends nothing: such a Server has no
// Cancel, and calling one would only report a refusal nobody can act on. The
// operator is told the device stays selected until its own timeout (backlog
// 1013), once, with the same title the 2.6 path's refused Cancel carries.
TEST_F(WriteModelTest, DecliningAfterAFlaggedSelectSendsNoCancel) {
  EXPECT_CALL(method_service_, Call(kControlNodeId, _, _, _))
      .WillRepeatedly([this](scada::NodeId, scada::NodeId method_id,
                             std::vector<scada::Variant>,
                             scada::ServiceContext) {
        called_methods_.push_back(method_id);
        return scada::MakeMethodCallResult(
            scada::Status{scada::StatusCode::Bad_WrongNodeId});
      });
  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce([](scada::ServiceContext, std::vector<scada::WriteValue>) {
        return scada::MakeAttributeResult<scada::StatusCode>(
            std::vector{scada::StatusCode::Good});
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  reviewer_.Complete(/*send=*/false);
  Drain(executor_);
  model.reset();
  Drain(executor_);

  EXPECT_THAT(called_methods_,
              ElementsAre(scada::data_items::id::DataItemControlType_Select));
  EXPECT_EQ(completion_, false);
  ASSERT_THAT(local_events_.events(), SizeIs(1));
  EXPECT_EQ(local_events_.events()[0]->severity, scada::kSeverityCritical);
  const std::u16string& text = local_events_.events()[0]->message.text;
  EXPECT_NE(text.find(u"Release of selection"), std::u16string::npos);
  EXPECT_NE(text.find(u"Output"), std::u16string::npos);
  EXPECT_NE(text.find(u"selection timeout"), std::u16string::npos);
}

// Closing the dialog with a 2.5-style select outstanding -- rather than
// answering No -- tells the operator the same thing (backlog 1013).
TEST_F(WriteModelTest, ClosingAfterAFlaggedSelectReportsTheHeldSelection) {
  ON_CALL(method_service_, Call)
      .WillByDefault([](scada::NodeId, scada::NodeId,
                        std::vector<scada::Variant>, scada::ServiceContext) {
        return scada::MakeMethodCallResult(
            scada::Status{scada::StatusCode::Bad_WrongNodeId});
      });
  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce([](scada::ServiceContext, std::vector<scada::WriteValue>) {
        return scada::MakeAttributeResult<scada::StatusCode>(
            std::vector{scada::StatusCode::Good});
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  EXPECT_THAT(local_events_.events(), IsEmpty());
  model.reset();
  Drain(executor_);

  ASSERT_THAT(local_events_.events(), SizeIs(1));
  EXPECT_NE(local_events_.events()[0]->message.text.find(u"Output"),
            std::u16string::npos);
}

// Backlog 1027: the operator closes the dialog while the 2.5-style select is
// still in flight. The device then accepts it with nobody left to operate,
// and such a Server has no Cancel -- so the operator is told the device stays
// selected, as closing after the answer does. Before the fix the answer went
// through CompleteWriteAsync, which dropped it with the model.
TEST_F(WriteModelTest,
       ClosingBeforeAFlaggedSelectAnswersReportsTheHeldSelection) {
  ON_CALL(method_service_, Call)
      .WillByDefault([](scada::NodeId, scada::NodeId,
                        std::vector<scada::Variant>, scada::ServiceContext) {
        return scada::MakeMethodCallResult(
            scada::Status{scada::StatusCode::Bad_WrongNodeId});
      });
  scada::base::AsyncCompletion select_answer{executor_};
  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce([&select_answer](scada::ServiceContext,
                                 std::vector<scada::WriteValue>) {
        return AwaitWriteAnswer(select_answer, scada::StatusCode::Good);
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  model.reset();
  Drain(executor_);
  EXPECT_THAT(local_events_.events(), IsEmpty());

  select_answer.Complete();
  Drain(executor_);

  ASSERT_THAT(local_events_.events(), SizeIs(1));
  const std::u16string& text = local_events_.events()[0]->message.text;
  EXPECT_NE(text.find(u"Release of selection"), std::u16string::npos);
  EXPECT_NE(text.find(u"Output"), std::u16string::npos);
  EXPECT_NE(text.find(u"selection timeout"), std::u16string::npos);
  // Nothing is asked of an operator who has closed the dialog.
  EXPECT_TRUE(dialog_service_.modes.empty());
}

// Backlog 1034: a 2.5.7 Server answers a select on a MODBUS device Good
// without contacting it ("Don't support Select"), so declining afterwards
// leaves nothing selected. Reporting the held selection there raised a false
// critical event on every declined command, and the prompt claimed the device
// was ready.
TEST_F(WriteModelTest, DecliningAFlaggedSelectOnAModbusDeviceReportsNothing) {
  scada::base::AsyncCompletion select_answer{executor_};
  ExpectLegacySelect(select_answer);

  auto model = CreateModel(
      /*two_staged=*/true, /*local_events=*/nullptr,
      AddDeviceChannel(scada::devices::id::ModbusDeviceType));
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  select_answer.Complete();
  Drain(executor_);

  ASSERT_THAT(reviewer_.reviews, SizeIs(1));
  // Still the operate stage, but nothing is waiting.
  EXPECT_NE(reviewer_.reviews.at(0).stage.find(u"Stage 2 of 2"),
            std::u16string::npos);
  EXPECT_EQ(reviewer_.reviews.at(0).stage.find(u"waiting"),
            std::u16string::npos);
  // The rest of the review is still there.
  EXPECT_NE(reviewer_.reviews.at(0).warning.find(u"cannot be undone"),
            std::u16string::npos);

  reviewer_.Complete(/*send=*/false);
  Drain(executor_);
  model.reset();
  Drain(executor_);

  EXPECT_EQ(completion_, false);
  EXPECT_THAT(local_events_.events(), IsEmpty());
}

// The same for a Classic OPC item closed while its flagged select is still in
// flight: the OPC tier never selects either, so the late Good holds nothing.
TEST_F(WriteModelTest,
       ClosingBeforeAFlaggedSelectOnAnOpcItemAnswersReportsNothing) {
  scada::base::AsyncCompletion select_answer{executor_};
  ExpectLegacySelect(select_answer);

  auto model = CreateModel(
      /*two_staged=*/true, /*local_events=*/nullptr,
      MakeNodeIdFormula(scada::NodeId{"Vendor.Server.1\\Group.Breaker1",
                                      scada::NamespaceIndexes::OPC}));
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  model.reset();
  Drain(executor_);
  select_answer.Complete();
  Drain(executor_);

  EXPECT_THAT(local_events_.events(), IsEmpty());
}

// An IEC 60870-5 object whose Output overrides the type to a set-point
// (`FLOAT:`) cannot select either, whatever its item type.
TEST_F(WriteModelTest, DecliningAFlaggedSetPointSelectReportsNothing) {
  scada::base::AsyncCompletion select_answer{executor_};
  ExpectLegacySelect(select_answer);

  auto model = CreateModel(
      /*two_staged=*/true, /*local_events=*/nullptr,
      AddDeviceChannel(scada::devices::id::Iec60870DeviceType, "FLOAT:1"));
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  select_answer.Complete();
  Drain(executor_);
  reviewer_.Complete(/*send=*/false);
  Drain(executor_);

  EXPECT_THAT(local_events_.events(), IsEmpty());
}

// A device that does select -- a single command on IEC 60870-5 -- still holds
// the 2.5.7 selection, so declining still reports it.
TEST_F(WriteModelTest,
       DecliningAFlaggedSelectOnASelectingDeviceStillReportsIt) {
  scada::base::AsyncCompletion select_answer{executor_};
  ExpectLegacySelect(select_answer);

  auto model = CreateModel(
      /*two_staged=*/true, /*local_events=*/nullptr,
      AddDeviceChannel(scada::devices::id::Iec60870DeviceType));
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  select_answer.Complete();
  Drain(executor_);
  EXPECT_NE(reviewer_.reviews.at(0).stage.find(u"waiting"),
            std::u16string::npos);
  reviewer_.Complete(/*send=*/false);
  Drain(executor_);

  ASSERT_THAT(local_events_.events(), SizeIs(1));
  EXPECT_NE(local_events_.events()[0]->message.text.find(u"selection timeout"),
            std::u16string::npos);
}

// The same close, but the Server refuses the flagged select: nothing was
// armed, so nothing is reported.
TEST_F(WriteModelTest, ClosingBeforeAFlaggedSelectIsRefusedReportsNothing) {
  ON_CALL(method_service_, Call)
      .WillByDefault([](scada::NodeId, scada::NodeId,
                        std::vector<scada::Variant>, scada::ServiceContext) {
        return scada::MakeMethodCallResult(
            scada::Status{scada::StatusCode::Bad_WrongNodeId});
      });
  scada::base::AsyncCompletion select_answer{executor_};
  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce([&select_answer](scada::ServiceContext,
                                 std::vector<scada::WriteValue>) {
        return AwaitWriteAnswer(select_answer, scada::StatusCode::Bad);
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  model.reset();
  select_answer.Complete();
  Drain(executor_);

  EXPECT_THAT(local_events_.events(), IsEmpty());
  EXPECT_TRUE(dialog_service_.modes.empty());
}

// A 2.5-style select the Server also refuses is reported once, as a refused
// select, and is not retried again.
TEST_F(WriteModelTest, RefusedFlaggedSelectIsReportedOnce) {
  ON_CALL(method_service_, Call)
      .WillByDefault([](scada::NodeId, scada::NodeId,
                        std::vector<scada::Variant>, scada::ServiceContext) {
        return scada::MakeMethodCallResult(
            scada::Status{scada::StatusCode::Bad_WrongNodeId});
      });
  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce([](scada::ServiceContext, std::vector<scada::WriteValue>) {
        return scada::MakeAttributeResult<scada::StatusCode>(
            std::vector{scada::StatusCode::Bad_WrongNodeId});
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  ASSERT_THAT(dialog_service_.modes, ElementsAre(MessageBoxMode::Error));
  EXPECT_NE(
      dialog_service_.messages.at(0).find(u"The command was not executed."),
      std::u16string::npos);
  dialog_service_.CompleteMessageBox();
  Drain(executor_);
  EXPECT_EQ(completion_, false);
}

// Any other refusal of the select still says the command was not executed.
TEST_F(WriteModelTest, RefusedSelectByTheDeviceSaysNotExecuted) {
  ON_CALL(method_service_, Call)
      .WillByDefault([](scada::NodeId, scada::NodeId,
                        std::vector<scada::Variant>, scada::ServiceContext) {
        return scada::MakeMethodCallResult(
            scada::Status{scada::StatusCode::Bad_Timeout});
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  ASSERT_THAT(dialog_service_.modes, ElementsAre(MessageBoxMode::Error));
  EXPECT_NE(
      dialog_service_.messages.at(0).find(u"The command was not executed."),
      std::u16string::npos);
  dialog_service_.CompleteMessageBox();
  Drain(executor_);
  EXPECT_EQ(completion_, false);
}

// The device answers the cancel after the dialog has closed and taken the
// model with it. The refusal still has to reach the operator.
TEST_F(WriteModelTest, CancelRefusedAfterTheDialogClosedIsStillReported) {
  scada::base::AsyncCompletion cancel_answer{executor_};
  ON_CALL(method_service_, Call)
      .WillByDefault([&](scada::NodeId, scada::NodeId method_id,
                         std::vector<scada::Variant>, scada::ServiceContext) {
        return method_id == scada::data_items::id::DataItemControlType_Cancel
                   ? AwaitRefusal(cancel_answer)
                   : scada::MakeMethodCallResult();
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  reviewer_.Complete(/*send=*/false);
  Drain(executor_);
  model.reset();

  EXPECT_THAT(local_events_.events(), IsEmpty());
  cancel_answer.Complete();
  Drain(executor_);

  EXPECT_THAT(local_events_.events(), SizeIs(1));
}

// Backlog 711: the operator closes the control dialog -- Escape, the window
// button -- while the select is still in flight. The device then accepts it
// with nobody left to operate or decline, and the selection used to stay armed
// until the outstation's own timeout.
TEST_F(WriteModelTest, SelectAcceptedAfterTheDialogClosedIsReleased) {
  scada::base::AsyncCompletion select_answer{executor_};
  ON_CALL(method_service_, Call)
      .WillByDefault([&](scada::NodeId, scada::NodeId method_id,
                         std::vector<scada::Variant>, scada::ServiceContext) {
        called_methods_.push_back(method_id);
        return method_id == scada::data_items::id::DataItemControlType_Select
                   ? AwaitAcceptance(select_answer)
                   : scada::MakeMethodCallResult();
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  model.reset();

  select_answer.Complete();
  Drain(executor_);

  EXPECT_THAT(called_methods_,
              ElementsAre(scada::data_items::id::DataItemControlType_Select,
                          scada::data_items::id::DataItemControlType_Cancel));
  // Nothing is asked of an operator who has closed the dialog.
  EXPECT_TRUE(dialog_service_.modes.empty());
}

// The dialog is destroyed while the operate confirmation is up, so the prompt
// is abandoned and never answers. The selection is released all the same.
TEST_F(WriteModelTest, DestroyedModelWithASelectionOutstandingReleasesIt) {
  ON_CALL(method_service_, Call)
      .WillByDefault([this](scada::NodeId, scada::NodeId method_id,
                            std::vector<scada::Variant>,
                            scada::ServiceContext) {
        called_methods_.push_back(method_id);
        return scada::MakeMethodCallResult();
      });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  ASSERT_THAT(reviewer_.reviews, SizeIs(1));

  model.reset();
  Drain(executor_);

  EXPECT_THAT(called_methods_,
              ElementsAre(scada::data_items::id::DataItemControlType_Select,
                          scada::data_items::id::DataItemControlType_Cancel));
}

// A refused select armed nothing, and a model that never selected has nothing
// to release: neither sends a Cancel.
TEST_F(WriteModelTest, NoSelectionHeldMeansNoCancelOnClose) {
  ON_CALL(method_service_, Call)
      .WillByDefault([this](scada::NodeId, scada::NodeId method_id,
                            std::vector<scada::Variant>,
                            scada::ServiceContext) {
        called_methods_.push_back(method_id);
        return scada::MakeMethodCallResult(
            method_id == scada::data_items::id::DataItemControlType_Select
                ? scada::Status{scada::StatusCode::Bad}
                : scada::Status{scada::StatusCode::Good});
      });

  auto model = CreateModel(/*two_staged=*/true);
  model.reset();
  Drain(executor_);
  EXPECT_THAT(called_methods_, IsEmpty());

  model = CreateModel(/*two_staged=*/true);
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  dialog_service_.CompleteMessageBox();
  Drain(executor_);
  model.reset();
  Drain(executor_);

  EXPECT_THAT(called_methods_,
              ElementsAre(scada::data_items::id::DataItemControlType_Select));
}

// A sign-out rebuilds the session, destroying the event sink, while a Cancel
// may still be waiting for the device. Its refusal then reports nothing
// rather than writing into a destroyed object.
TEST_F(WriteModelTest, CancelRefusedAfterTheEventSinkIsGoneReportsNothing) {
  scada::base::AsyncCompletion cancel_answer{executor_};
  ON_CALL(method_service_, Call)
      .WillByDefault([&](scada::NodeId, scada::NodeId method_id,
                         std::vector<scada::Variant>, scada::ServiceContext) {
        return method_id == scada::data_items::id::DataItemControlType_Cancel
                   ? AwaitRefusal(cancel_answer)
                   : scada::MakeMethodCallResult();
      });

  auto session_events = std::make_unique<LocalEvents>();
  auto model = CreateModel(/*two_staged=*/true, session_events.get());
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  model.reset();  // The dialog closes with the selection outstanding.
  Drain(executor_);
  session_events.reset();  // The session is torn down.

  cancel_answer.Complete();
  Drain(executor_);  // Under ASan, a report here is a heap-use-after-free.

  EXPECT_THAT(local_events_.events(), IsEmpty());
}

TEST_F(WriteModelTest, ControlCommandConfirmationReviewsPresentAndCommand) {
  // control_confirmation defaults to true, so a control write must be
  // reviewed deliberately before anything reaches the device.
  auto model = CreateModel();
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  // The review is shown — not a yes/no question box — and, crucially, nothing
  // is written yet: StrictMock<MockAttributeService> would fail on an
  // unexpected Write().
  ASSERT_THAT(reviewer_.reviews, SizeIs(1));
  EXPECT_TRUE(dialog_service_.modes.empty());
  EXPECT_FALSE(completion_.has_value());

  // It reviews the object, the commanded value, the stage, and warns that the
  // action is irreversible — not a bare "Switch X to Y?" (backlog 1118).
  const ControlReview& review = reviewer_.reviews.at(0);
  EXPECT_EQ(review.title, model->GetWindowTitle());
  EXPECT_EQ(review.object, u"Output");
  EXPECT_NE(review.command.find(u"42"), std::u16string::npos);
  EXPECT_NE(review.stage.find(u"One stage"), std::u16string::npos);
  EXPECT_NE(review.warning.find(u"cannot be undone remotely"),
            std::u16string::npos);
  EXPECT_FALSE(review.time.empty());
}

// Sending from the review writes the value the review showed.
TEST_F(WriteModelTest, SendingFromTheReviewWritesTheCommand) {
  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce(DoAll(
          WithArg<1>([](const auto& inputs) {
            ASSERT_EQ(inputs.size(), 1u);
            EXPECT_DOUBLE_EQ(inputs[0].value.template get<double>(), 42.0);
          }),
          [](scada::ServiceContext, std::vector<scada::WriteValue>) {
            return scada::MakeAttributeResult<scada::StatusCode>(
                std::vector{scada::StatusCode::Good});
          }));

  auto model = CreateModel();
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  reviewer_.Complete(/*send=*/true);
  Drain(executor_);

  EXPECT_EQ(completion_, true);
}

// Cancelling the review sends nothing and leaves the dialog open.
TEST_F(WriteModelTest, CancellingTheReviewSendsNothing) {
  auto model = CreateModel();
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);
  reviewer_.Complete(/*send=*/false);
  Drain(executor_);

  EXPECT_EQ(completion_, false);
  EXPECT_TRUE(model->GetStatusText().empty());
}

// Backlog 1118: only an administrator may switch confirmation off. An
// operator's session reviews every command even when the profile says "off"
// — a stored "off" from an administrator's session never reaches them.
TEST_F(WriteModelTest, AnOperatorCannotSwitchConfirmationOff) {
  profile_.control_confirmation = false;
  may_skip_confirmation_ = false;

  auto model = CreateModel();
  EXPECT_TRUE(model->IsConfirmationRequired());
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  // Reviewed, and nothing written (StrictMock).
  EXPECT_THAT(reviewer_.reviews, SizeIs(1));
  EXPECT_FALSE(completion_.has_value());
}

// An administrator keeps the switch, for commissioning and test benches.
TEST_F(WriteModelTest, AnAdministratorMaySwitchConfirmationOff) {
  profile_.control_confirmation = false;
  may_skip_confirmation_ = true;

  EXPECT_FALSE(CreateModel()->IsConfirmationRequired());
}

// Manual input is not a command to the field and was never reviewed.
TEST_F(WriteModelTest, ManualInputIsNeverReviewed) {
  may_skip_confirmation_ = false;
  CreateModel();  // Seeds the item.
  auto manual = std::make_shared<WriteModel>(
      WriteContext{.executor_ = executor_,
                   .timed_data_service_ = timed_data_service_,
                   .node_id_ = kDataItemId,
                   .profile_ = profile_,
                   .manual_ = true});

  EXPECT_FALSE(manual->IsConfirmationRequired());
}

// A model nobody installed a reviewer on declines rather than sending an
// unreviewed command.
TEST_F(WriteModelTest, WithoutAReviewerTheCommandIsDeclined) {
  auto model = CreateModel();
  model->set_control_reviewer({});
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  // StrictMock: no Write reached the server.
  EXPECT_EQ(completion_, false);
}

// The review names who the command is logged against, and when (principle
// §7: always show who the action is logged against).
TEST_F(WriteModelTest, ReviewNamesTheOperatorAndTheTime) {
  user_id_ = scada::NodeId{9300, 1};
  node_service_.Add(
      scada::NodeState{.node_id = user_id_,
                       .node_class = scada::NodeClass::Object,
                       .type_definition_id = scada::NodeId{9301, 1},
                       .attributes = {.display_name = u"Operator Ivanov"}});

  auto model = CreateModel();
  const scada::Time now =
      std::chrono::sys_days{std::chrono::year{2026} / 11 / 23} +
      std::chrono::hours{14};
  const ControlReview review =
      model->GetControlReview(42.0, /*second_stage=*/false, now);

  EXPECT_EQ(review.operator_name, u"Operator Ivanov");
  EXPECT_EQ(review.time, UtfConvert<char16_t>(FormatTime(
                             now, TIME_FORMAT_DATE | TIME_FORMAT_TIME)));
}

// UX audit 2026-11-25, item 3.1: the review names the object by its path, so
// two substations' identically named breakers cannot be confused at the last
// screen before the command.
TEST_F(WriteModelTest, ReviewNamesTheObjectByItsPath) {
  const scada::NodeId group_id{9500, 1};
  node_service_.Add(
      scada::NodeState{.node_id = scada::data_items::id::DataGroupType,
                       .node_class = scada::NodeClass::ObjectType,
                       .attributes = {.display_name = u"Data group"}});
  node_service_.Add(scada::NodeState{
      .node_id = group_id,
      .node_class = scada::NodeClass::Object,
      .type_definition_id = scada::data_items::id::DataGroupType,
      .attributes = {.display_name = u"North substation"}});
  parent_id_ = group_id;

  auto model = CreateModel();
  EXPECT_EQ(model->GetObjectPath(), u"North substation : Output");
  EXPECT_EQ(
      model->GetControlReview(1.0, /*second_stage=*/false, scada::Now()).object,
      u"North substation : Output");
}

TEST_F(WriteModelTest, AnAnonymousSessionIsNamedAsSuch) {
  auto model = CreateModel();

  EXPECT_EQ(model->GetOperatorName(), u"anonymous");
}

// Backlog 1118: a blocked command names the interlock that blocks it. The
// condition row used to read «Условие: Нарушено» and leave the operator to
// find the formula in the configuration.
class WriteModelConditionTest : public WriteModelTest {
 protected:
  static constexpr std::string_view kInterlock = "TIT.211 > 70";

  WriteModelConditionTest() {
    output_condition_ = std::string{kInterlock};
    ON_CALL(timed_data_service_, GetFormulaTimedData(Eq(kInterlock), _))
        .WillByDefault(Return(condition_data_));
    // An expression's title is its formula over the operands' aliases.
    ON_CALL(*condition_data_, GetTitle())
        .WillByDefault(Return(u"TIT.211 > 70"));
  }

  void SetCondition(bool value, scada::Qualifier qualifier = {}) {
    ON_CALL(*condition_data_, GetDataValue())
        .WillByDefault(Return(scada::DataValue{scada::Variant{value}, qualifier,
                                               scada::Time{}, scada::Time{}}));
  }

  std::shared_ptr<NiceMock<MockTimedData>> condition_data_ =
      std::make_shared<NiceMock<MockTimedData>>();
};

TEST_F(WriteModelConditionTest, AMetConditionReadsSatisfied) {
  SetCondition(true);
  auto model = CreateModel();

  ASSERT_TRUE(model->has_condition());
  EXPECT_TRUE(model->IsConditionOk());
  EXPECT_EQ(model->GetConditionText(), u"Satisfied");
}

TEST_F(WriteModelConditionTest, AnUnmetConditionNamesTheInterlock) {
  SetCondition(false);
  auto model = CreateModel();

  EXPECT_FALSE(model->IsConditionOk());
  EXPECT_EQ(model->GetConditionText(), u"Required: TIT.211 > 70");
}

// UX audit 2026-11-25, item 3.2: the blocked command says what each input of
// the interlock reads now, by name and by the alias the formula uses, so the
// operator need not look «TIT.211» up to learn which value blocks them.
TEST_F(WriteModelConditionTest, AnUnmetConditionSaysWhatItsInputsReadNow) {
  const scada::NodeId input_id{9400, 1};
  node_service_.Add(
      scada::NodeState{.node_id = input_id,
                       .node_class = scada::NodeClass::Variable,
                       .type_definition_id = kDataItemTypeId,
                       .attributes = {.display_name = u"Oil temperature"}});
  auto input = std::make_shared<NiceMock<MockTimedData>>();
  ON_CALL(*input, GetNode()).WillByDefault([this, input_id] {
    return node_service_.GetNode(input_id);
  });
  ON_CALL(*input, GetFormula(_)).WillByDefault(Return("TIT.211"));
  ON_CALL(*input, GetDataValue())
      .WillByDefault(
          Return(scada::DataValue{scada::Variant{58.0}, scada::Qualifier{},
                                  scada::Time{}, scada::Time{}}));
  const std::vector<std::shared_ptr<TimedData>> operands{input};
  ON_CALL(*condition_data_, GetOperands())
      .WillByDefault(
          Return(std::span<const std::shared_ptr<TimedData>>{operands}));
  SetCondition(false);
  auto model = CreateModel();

  const std::u16string text = model->GetConditionText();
  EXPECT_TRUE(
      text.starts_with(u"Required: TIT.211 > 70; now: "
                       u"Oil temperature (TIT.211) = 58"))
      << UtfConvert<char>(text);
}

// A condition whose operands are invalid blocks the command too, but for a
// different reason: it cannot be evaluated, which is not the same as false.
TEST_F(WriteModelConditionTest, AnInvalidConditionSaysItCannotBeEvaluated) {
  SetCondition(true, scada::Qualifier{scada::Qualifier::BAD});
  auto model = CreateModel();

  EXPECT_FALSE(model->IsConditionOk());
  EXPECT_EQ(model->GetConditionText(), u"Cannot be evaluated: TIT.211 > 70");
}

// The confirmation is the last screen before an irreversible command, so it
// must carry the same quality marks as the dialog: «?» for BAD (backlog 908)
// and the status letters for a reading the server marks offline or stale
// without BAD (backlog 923).
TEST_F(WriteModelTest, ConfirmationMarksAnInvalidPresentValue) {
  ON_CALL(*timed_data_, GetDataValue())
      .WillByDefault(Return(scada::DataValue{
          scada::Variant{5.0},
          scada::Qualifier{scada::Qualifier::BAD | scada::Qualifier::OFFLINE},
          scada::Time{}, scada::Time{}}));

  auto model = CreateModel();
  model->Write(42.0, /*lock=*/false);
  Drain(executor_);

  ASSERT_THAT(reviewer_.reviews, SizeIs(1));
  const std::u16string& present = reviewer_.reviews.at(0).present;
  EXPECT_NE(present.find(u"5?"), std::u16string::npos) << present;
  EXPECT_NE(present.find(u"[С]"), std::u16string::npos) << present;
}

TEST_F(WriteModelTest, OfflineValueWithoutBadShowsItsStatusLetter) {
  ON_CALL(*timed_data_, GetDataValue())
      .WillByDefault(Return(scada::DataValue{
          scada::Variant{5.0}, scada::Qualifier{scada::Qualifier::STALE},
          scada::Time{}, scada::Time{}}));

  auto model = CreateModel();

  EXPECT_NE(model->GetCurrentValue(/*formatted=*/true).find(u"[У]"),
            std::u16string::npos);
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

// Backlog 849: "Report successful control" is what the setting promises, so
// with it on a completed command leaves a record in the event panel. It used
// to govern only configuration edits, and control closed silently.
TEST_F(WriteModelTest, SuccessfulControlIsReportedWhenTheSettingIsOn) {
  profile_.control_confirmation = false;
  profile_.show_write_ok = true;
  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce([&](scada::ServiceContext, std::vector<scada::WriteValue>)
                    -> scada::CoStatusOr<std::vector<scada::StatusCode>> {
        co_return std::vector{scada::StatusCode::Good};
      });

  auto model = CreateModel();
  model->Write(7.0, /*lock=*/false);
  Drain(executor_);

  ASSERT_THAT(local_events_.events(), SizeIs(1));
  EXPECT_EQ(local_events_.events()[0]->severity, scada::kSeverityNormal);
  EXPECT_NE(local_events_.events()[0]->message.text.find(u"Output"),
            std::u16string::npos);
}

TEST_F(WriteModelTest, SuccessfulControlIsSilentWhenTheSettingIsOff) {
  profile_.control_confirmation = false;
  profile_.show_write_ok = false;
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
  EXPECT_THAT(local_events_.events(), IsEmpty());
}

// The select half of a two-staged command is not the command; only the operate
// that follows it completes anything, so the select reports nothing.
TEST_F(WriteModelTest, TwoStagedControlReportsOnceAfterTheOperate) {
  profile_.control_confirmation = false;
  profile_.show_write_ok = true;
  ON_CALL(method_service_, Call)
      .WillByDefault(
          [](scada::NodeId, scada::NodeId, std::vector<scada::Variant>,
             scada::ServiceContext) { return scada::MakeMethodCallResult(); });

  auto model = CreateModel(/*two_staged=*/true);
  model->Write(1.0, /*lock=*/false);
  Drain(executor_);

  ASSERT_TRUE(completion_.has_value());
  EXPECT_TRUE(*completion_);
  EXPECT_THAT(local_events_.events(), SizeIs(1));
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

namespace {

// «12,5» typed on a Russian keyboard used to be refused as "Incorrect floating
// point value" (backlog 912).
TEST(ParseOperatorNumberTest, AcceptsDecimalPointAndDecimalComma) {
  EXPECT_EQ(ParseOperatorNumber(u"12.5"), 12.5);
  EXPECT_EQ(ParseOperatorNumber(u"12,5"), 12.5);
  EXPECT_EQ(ParseOperatorNumber(u" -3,25 "), -3.25);
  EXPECT_EQ(ParseOperatorNumber(u"+7"), 7.0);
  EXPECT_EQ(ParseOperatorNumber(u"1e3"), 1000.0);
}

TEST(ParseOperatorNumberTest, RejectsWhatIsNotANumber) {
  EXPECT_EQ(ParseOperatorNumber(u""), std::nullopt);
  EXPECT_EQ(ParseOperatorNumber(u"12,5,1"), std::nullopt);
  EXPECT_EQ(ParseOperatorNumber(u"12 кВ"), std::nullopt);
  EXPECT_EQ(ParseOperatorNumber(u"abc"), std::nullopt);
}

}  // namespace
