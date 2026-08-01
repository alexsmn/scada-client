#include "user_access/users_grid.h"

#include "base/awaitable.h"
#include "common/node_state.h"
#include "model/security_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/test/fake_node_service.h"
#include "scada/access_rights.h"
#include "scada/node_id.h"
#include "scada/standard_node_ids.h"
#include "scada/variant.h"

#include <gtest/gtest.h>

#include <boost/asio/io_context.hpp>

#include <optional>

namespace {

class UsersGridTest : public ::testing::Test {
 protected:
  UsersGridTest() {
    folder_ = node_service_.Add(scada::NodeState{
        .node_id = kFolderId, .node_class = scada::NodeClass::Object});
    node_service_.Add(
        scada::NodeState{.node_id = scada::security::id::UserType,
                         .node_class = scada::NodeClass::ObjectType});
  }

  // Registers one UserType instance under the Users folder. Each property is
  // seeded only when the optional holds a Variant; an *empty* Variant registers
  // the property node with nothing ever delivered for it, which is the state
  // under test. Seeded directly rather than via set_property(), which treats an
  // empty Variant as a removal.
  void AddUser(scada::UInt32 id,
               std::optional<scada::Variant> access,
               std::optional<scada::Variant> multi_sessions) {
    scada::NodeState user{.node_id = scada::NodeId{id, 1},
                          .node_class = scada::NodeClass::Object,
                          .type_definition_id = scada::security::id::UserType,
                          .parent_id = kFolderId,
                          .reference_type_id = scada::id::Organizes,
                          .attributes = {.display_name = u"user"}};
    if (access) {
      user.properties.emplace_back(scada::security::id::UserType_AccessRights,
                                   std::move(*access));
    }
    if (multi_sessions) {
      user.properties.emplace_back(scada::security::id::UserType_MultiSessions,
                                   std::move(*multi_sessions));
    }
    node_service_.Add(std::move(user));
  }

  std::vector<UserGridRow> Build() {
    return RunAwaitable(io_, [this]() -> Awaitable<std::vector<UserGridRow>> {
      co_return co_await BuildUsersGrid(io_.get_executor(), folder_);
    });
  }

  static scada::Variant Rights(bool configure, bool control) {
    scada::Int32 bits = 0;
    if (configure)
      bits |= scada::AccessRightBit(scada::AccessRight::kConfigure);
    if (control)
      bits |= scada::AccessRightBit(scada::AccessRight::kControl);
    return scada::Variant{bits};
  }

  static constexpr scada::NodeId kFolderId{7000, 1};

  boost::asio::io_context io_;
  FakeNodeService node_service_;
  NodeRef folder_;
};

// Regression: an AccessRights that was never delivered used to be folded into a
// zero bitmask by get_or<Int32>(0). Zero is a valid bitmask meaning Observer
// with view only, so the grid stated a role it had never read — the same defect
// the RBAC panel carried, and the one docs/client/ux/principles.md §5 forbids.
TEST_F(UsersGridTest, UndeliveredAccessRightsDoesNotReadAsObserver) {
  AddUser(1, scada::Variant{}, scada::Variant{true});

  const std::vector<UserGridRow> rows = Build();
  ASSERT_EQ(rows.size(), 1u);
  EXPECT_NE(rows[0].role, UserRole::kObserver);
  EXPECT_EQ(rows[0].role, UserRole::kUnknown);
  // The reads are independent: a resolved MultiSessions still comes through.
  EXPECT_EQ(rows[0].multi_sessions, std::optional<bool>{true});
}

// The same rule for MultiSessions: false is a real answer ("single session"),
// so an undelivered flag must not borrow it.
TEST_F(UsersGridTest, UndeliveredMultiSessionsDoesNotReadAsSingle) {
  AddUser(1, Rights(true, true), scada::Variant{});

  const std::vector<UserGridRow> rows = Build();
  ASSERT_EQ(rows.size(), 1u);
  EXPECT_EQ(rows[0].multi_sessions, std::nullopt);
  EXPECT_EQ(rows[0].role, UserRole::kAdministrator);
}

// A user whose properties never materialized at all — both aggregate lookups
// miss, and neither may fall back to a default.
TEST_F(UsersGridTest, AbsentPropertiesReadAsUnknown) {
  AddUser(1, std::nullopt, std::nullopt);

  const std::vector<UserGridRow> rows = Build();
  ASSERT_EQ(rows.size(), 1u);
  EXPECT_EQ(rows[0].role, UserRole::kUnknown);
  EXPECT_EQ(rows[0].multi_sessions, std::nullopt);
}

// The counterpart the fix must not disturb.
TEST_F(UsersGridTest, DeliveredValuesResolveToRealRoles) {
  AddUser(1, Rights(true, true), scada::Variant{true});
  AddUser(2, Rights(false, true), scada::Variant{false});

  std::vector<UserGridRow> rows = Build();
  ASSERT_EQ(rows.size(), 2u);
  std::ranges::sort(rows, {}, &UserGridRow::node_id);

  EXPECT_EQ(rows[0].role, UserRole::kAdministrator);
  EXPECT_EQ(rows[0].multi_sessions, std::optional<bool>{true});
  EXPECT_EQ(rows[1].role, UserRole::kOperator);
  EXPECT_EQ(rows[1].multi_sessions, std::optional<bool>{false});
}

// A genuine zero bitmask is an answer, not an absence — it must keep reading as
// Observer, which is what makes the undelivered case worth telling apart.
TEST_F(UsersGridTest, ZeroAccessRightsStillReadsAsObserver) {
  AddUser(1, Rights(false, false), scada::Variant{false});

  const std::vector<UserGridRow> rows = Build();
  ASSERT_EQ(rows.size(), 1u);
  EXPECT_EQ(rows[0].role, UserRole::kObserver);
  EXPECT_EQ(rows[0].multi_sessions, std::optional<bool>{false});
}

}  // namespace
