#include "services/server_kind.h"

#include "base/test/awaitable_test.h"
#include "base/test/test_executor.h"
#include "scada/attribute_service.h"
#include "scada/session_service_mock.h"
#include "scada/standard_node_ids.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace {

using scada::model::ServerKind;

// Answers every Read with one fixed DataValue, or a whole-call failure, and
// records the node it was asked for.
class FixedAttributeService final : public scada::AttributeService {
 public:
  scada::CoStatusOr<std::vector<scada::DataValue>> Read(
      scada::ServiceContext,
      std::vector<scada::ReadValueId> inputs) override {
    for (const scada::ReadValueId& input : inputs)
      read_node_ids.push_back(input.node_id);
    return Answer(call_status, value, inputs.size());
  }

  scada::CoStatusOr<std::vector<scada::StatusCode>> Write(
      scada::ServiceContext,
      std::vector<scada::WriteValue>) override {
    return Fail();
  }

  std::optional<scada::Status> call_status;
  scada::DataValue value;
  std::vector<scada::NodeId> read_node_ids;

 private:
  static scada::CoStatusOr<std::vector<scada::DataValue>> Answer(
      std::optional<scada::Status> call_status,
      scada::DataValue value,
      size_t count) {
    if (call_status)
      co_return *call_status;
    co_return std::vector<scada::DataValue>(count, value);
  }

  static scada::CoStatusOr<std::vector<scada::StatusCode>> Fail() {
    co_return scada::Status{scada::StatusCode::Bad};
  }
};

class ServerKindTest : public ::testing::Test {
 protected:
  ServerKind Probe() {
    return WaitAwaitable(executor_, ReadServerKind(attributes_));
  }

  void Publish(std::vector<scada::String> uris) {
    attributes_->value = scada::MakeReadResult(std::move(uris));
  }

  TestExecutor executor_;
  std::shared_ptr<FixedAttributeService> attributes_ =
      std::make_shared<FixedAttributeService>();
};

TEST_F(ServerKindTest, ReadsTheServersNamespaceArray) {
  Publish({"http://opcfoundation.org/UA/"});
  Probe();
  EXPECT_EQ(attributes_->read_node_ids,
            (std::vector<scada::NodeId>{
                scada::NodeId{scada::id::Server_NamespaceArray}}));
}

TEST_F(ServerKindTest, AServerPublishingOurNamespaceIsOurs) {
  Publish({"http://opcfoundation.org/UA/", "urn:host:scada-iec104",
           "http://telecontrol.ru/opcua/scada"});
  EXPECT_EQ(Probe(), ServerKind::kOurs);
}

TEST_F(ServerKindTest, AServerWithoutOurNamespaceIsForeign) {
  Publish({"http://opcfoundation.org/UA/", "urn:open62541.server.application",
           "http://example.com/vendor"});
  EXPECT_EQ(Probe(), ServerKind::kForeign);
}

// Nothing to go on is not evidence of a foreign server: the Client keeps the
// behaviour it had before it asked.
TEST_F(ServerKindTest, AnUnanswerableReadDecidesOurs) {
  attributes_->call_status = scada::Status{scada::StatusCode::Bad_Timeout};
  EXPECT_EQ(Probe(), ServerKind::kOurs);

  attributes_->call_status.reset();
  attributes_->value = scada::MakeReadError(scada::StatusCode::Bad);
  EXPECT_EQ(Probe(), ServerKind::kOurs);

  attributes_->value = scada::MakeReadResult(scada::String{"not an array"});
  EXPECT_EQ(Probe(), ServerKind::kOurs);

  Publish({});
  EXPECT_EQ(Probe(), ServerKind::kOurs);
}

TEST(ServerKindSessionServiceTest, AnswersIsScadaFromTheKindItIsTold) {
  auto inner = std::make_shared<testing::NiceMock<scada::MockSessionService>>();
  ON_CALL(*inner, IsScada()).WillByDefault(testing::Return(false));
  ON_CALL(*inner, GetAccessRights()).WillByDefault(testing::Return(42u));
  ServerKindSessionService session{inner};

  // Before the probe has answered: the behaviour from before it existed.
  EXPECT_TRUE(session.IsScada());

  session.set_server_kind(ServerKind::kForeign);
  EXPECT_FALSE(session.IsScada());

  session.set_server_kind(ServerKind::kOurs);
  EXPECT_TRUE(session.IsScada());

  // Everything else is the wrapped session's.
  EXPECT_EQ(session.GetAccessRights(), 42u);
}

}  // namespace
