#include "modules/write/write_model.h"

#include "base/async_completion.h"
#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "node_service/static/static_node_service.h"
#include "profile/profile.h"
#include "scada/attribute_service_mock.h"
#include "timed_data/timed_data_service_mock.h"

#include <gmock/gmock.h>

#include <optional>

using namespace testing;

namespace {

constexpr scada::NodeId kDataItemTypeId{9001, 1};
constexpr scada::NodeId kDataItemId{9002, 1};

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
        std::make_unique<base::AsyncCompletion>(executor_);
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
  std::unique_ptr<base::AsyncCompletion> message_box_completion_;
};

class WriteModelTest : public Test {
 protected:
  WriteModelTest()
      : node_service_{scada::services{.attribute_service =
                                          &attribute_service_}},
        dialog_service_{executor_} {
    node_service_.Add(scada::NodeState{}
                          .set_node_id(kDataItemTypeId)
                          .set_node_class(scada::NodeClass::VariableType)
                          .set_display_name(u"Output type"));
    node_service_.Add(
        scada::NodeState{}
            .set_node_id(kDataItemId)
            .set_node_class(scada::NodeClass::Variable)
            .set_type_definition_id(kDataItemTypeId)
            .set_display_name(u"Output")
            .set_property(data_items::id::DataItemType_OutputTwoStaged, false)
            .set_property(data_items::id::DataItemType_Locked, false));

    ON_CALL(timed_data_service_, GetFormulaTimedData(_, _))
        .WillByDefault(Return(timed_data_));
    ON_CALL(*timed_data_, GetNode())
        .WillByDefault(Return(node_service_.GetNode(kDataItemId)));
    ON_CALL(*timed_data_, GetTitle()).WillByDefault(Return(u"Output"));
  }

  std::shared_ptr<WriteModel> CreateModel() {
    auto model = std::make_shared<WriteModel>(
        WriteContext{executor_, timed_data_service_, kDataItemId, profile_,
                     /*manual_=*/false});
    model->set_dialog_service(&dialog_service_);
    model->status_change_handler = [this] { ++status_changes_; };
    model->completion_handler = [this](bool ok) { completion_ = ok; };
    return model;
  }

  TestExecutor executor_;
  StrictMock<scada::MockAttributeService> attribute_service_;
  NiceMock<MockTimedDataService> timed_data_service_;
  std::shared_ptr<NiceMock<MockTimedData>> timed_data_ =
      std::make_shared<NiceMock<MockTimedData>>();
  StaticNodeService node_service_;
  Profile profile_;
  RecordingDialogService dialog_service_;
  int status_changes_ = 0;
  std::optional<bool> completion_;
};

}  // namespace

TEST_F(WriteModelTest, SuccessfulWriteCompletesAfterAttributeCallback) {
  profile_.control_confirmation = false;
  base::AsyncCompletion completion{executor_};
  std::optional<scada::StatusOr<std::vector<scada::StatusCode>>> result;

  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce(DoAll(
          WithArg<1>([](const auto& inputs) {
            ASSERT_EQ(inputs->size(), 1u);
            EXPECT_EQ((*inputs)[0].node_id, kDataItemId);
            EXPECT_EQ((*inputs)[0].attribute_id, scada::AttributeId::Value);
            EXPECT_DOUBLE_EQ((*inputs)[0].value.template get<double>(), 42.0);
            EXPECT_FALSE((*inputs)[0].flags.select());
          }),
          Invoke([&](scada::ServiceContext,
                     std::shared_ptr<const std::vector<scada::WriteValue>>)
                     -> Awaitable<scada::StatusOr<
                         std::vector<scada::StatusCode>>> {
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

TEST_F(WriteModelTest, FailedWriteReportsErrorThenCompletes) {
  profile_.control_confirmation = false;
  base::AsyncCompletion completion{executor_};
  std::optional<scada::StatusOr<std::vector<scada::StatusCode>>> result;

  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce([&](scada::ServiceContext,
                    std::shared_ptr<const std::vector<scada::WriteValue>>)
                    -> Awaitable<scada::StatusOr<
                        std::vector<scada::StatusCode>>> {
        co_await completion.Wait();
        co_return std::move(*result);
      });

  auto model = CreateModel();
  model->Write(7.0, /*lock=*/false);
  Drain(executor_);

  result = std::vector{scada::StatusCode::Bad};
  completion.Complete();
  Drain(executor_);

  ASSERT_THAT(dialog_service_.modes, ElementsAre(MessageBoxMode::Error));
  EXPECT_FALSE(completion_.has_value());

  dialog_service_.CompleteMessageBox();
  Drain(executor_);

  EXPECT_EQ(completion_, true);
}

TEST_F(WriteModelTest, DestroyedModelDropsPendingWriteCompletion) {
  profile_.control_confirmation = false;
  base::AsyncCompletion completion{executor_};
  std::optional<scada::StatusOr<std::vector<scada::StatusCode>>> result;

  EXPECT_CALL(attribute_service_, Write(_, _))
      .WillOnce([&](scada::ServiceContext,
                    std::shared_ptr<const std::vector<scada::WriteValue>>)
                    -> Awaitable<scada::StatusOr<
                        std::vector<scada::StatusCode>>> {
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
