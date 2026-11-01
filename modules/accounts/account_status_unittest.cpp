#include "accounts/account_status.h"

#include "base/awaitable.h"
#include "common/node_state.h"
#include "node_service/test/fake_node_service.h"
#include "scada/method_service.h"
#include "scada/standard_node_ids.h"

#include <boost/asio/io_context.hpp>
#include <gtest/gtest.h>

#include <optional>

namespace {

// Records the one ModifyUser call the command makes and answers Good.
class RecordingMethodService : public scada::MethodService {
 public:
  scada::CoStatusOr<scada::CallResult> Call(
      scada::NodeId node_id,
      scada::NodeId method_id,
      std::vector<scada::Variant> arguments,
      scada::ServiceContext context) override {
    object_id = node_id;
    this->method_id = method_id;
    this->arguments = arguments;
    co_return scada::MakeCallResult(scada::StatusCode::Good);
  }

  scada::NodeId object_id;
  scada::NodeId method_id;
  std::vector<scada::Variant> arguments;
};

scada::Status RunSetAccountDisabled(NodeService& node_service,
                                    scada::UserConfiguration current,
                                    bool disabled) {
  boost::asio::io_context io;
  std::optional<scada::Status> status;
  RunAwaitable(io, [&]() -> Awaitable<void> {
    status.emplace(co_await SetAccountDisabled(node_service, u"ivanov", current,
                                               disabled));
  });
  return *status;
}

// Enable / Disable Account is the standard ModifyUser on the UserManagement
// object, changing the Disabled bit and nothing else (OPC UA Part 18 §5.2.7) —
// so an account's NoDelete survives being disabled and enabled (backlog 149).
TEST(AccountStatusTest, FlipsOnlyTheDisabledBit) {
  RecordingMethodService methods;
  FakeNodeService node_service{scada::services{.method_service = &methods}};
  const scada::NodeId user_management{
      scada::id::Server_ServerConfiguration_UserManagement, 0};
  node_service.Add(scada::NodeState{.node_id = user_management,
                                    .node_class = scada::NodeClass::Object});

  EXPECT_TRUE(RunSetAccountDisabled(node_service,
                                    scada::UserConfiguration::kNoDelete,
                                    /*disabled=*/true));
  EXPECT_EQ(methods.object_id, user_management);
  EXPECT_EQ(methods.method_id,
            (scada::NodeId{scada::id::UserManagement_ModifyUser, 0}));
  ASSERT_EQ(methods.arguments.size(), 7u);
  EXPECT_EQ(methods.arguments[0],
            scada::Variant{scada::LocalizedText{u"ivanov"}});
  EXPECT_EQ(methods.arguments[1], scada::Variant{false});  // modify password
  EXPECT_EQ(methods.arguments[3], scada::Variant{true});   // modify mask
  EXPECT_EQ(methods.arguments[4], scada::Variant{static_cast<scada::UInt32>(
                                      scada::UserConfiguration::kNoDelete |
                                      scada::UserConfiguration::kDisabled)});

  EXPECT_TRUE(RunSetAccountDisabled(
      node_service,
      scada::UserConfiguration::kNoDelete | scada::UserConfiguration::kDisabled,
      /*disabled=*/false));
  EXPECT_EQ(methods.arguments[4], scada::Variant{static_cast<scada::UInt32>(
                                      scada::UserConfiguration::kNoDelete)});
}

}  // namespace
