#include "modules/table/table_double_click.h"

#include "scada/access_rights.h"
#include "scada/session_service_mock.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

// Backlog 1069: a blinking row is acknowledged on double-click only when the
// user may acknowledge; otherwise the double-click opens the row as usual.
class TableDoubleClickTest : public Test {
 protected:
  void SetAccessRights(std::uint32_t access_rights) {
    ON_CALL(session_service_, GetAccessRights())
        .WillByDefault(Return(access_rights));
  }

  NiceMock<scada::MockSessionService> session_service_;
};

TEST_F(TableDoubleClickTest, OperatorAcknowledgesABlinkingRow) {
  SetAccessRights(scada::AccessRightBit(scada::AccessRight::kControl));
  EXPECT_EQ(
      ChooseTableDoubleClickAction(/*row_blinking=*/true, session_service_),
      TableDoubleClickAction::kAcknowledge);
}

TEST_F(TableDoubleClickTest, ConfigureRightAcknowledgesABlinkingRow) {
  SetAccessRights(scada::AccessRightBit(scada::AccessRight::kConfigure));
  EXPECT_EQ(
      ChooseTableDoubleClickAction(/*row_blinking=*/true, session_service_),
      TableDoubleClickAction::kAcknowledge);
}

TEST_F(TableDoubleClickTest, ViewOnlyUserOpensABlinkingRow) {
  SetAccessRights(0);
  EXPECT_EQ(
      ChooseTableDoubleClickAction(/*row_blinking=*/true, session_service_),
      TableDoubleClickAction::kDefaultCommand);
}

TEST_F(TableDoubleClickTest, AnonymousSessionOpensABlinkingRow) {
  SetAccessRights(scada::AccessRightBit(scada::AccessRight::kControl));
  ON_CALL(session_service_, IsAnonymous()).WillByDefault(Return(true));
  EXPECT_EQ(
      ChooseTableDoubleClickAction(/*row_blinking=*/true, session_service_),
      TableDoubleClickAction::kDefaultCommand);
}

TEST_F(TableDoubleClickTest, QuietRowOpensWhateverTheRights) {
  SetAccessRights(scada::AccessRightBit(scada::AccessRight::kControl));
  EXPECT_EQ(
      ChooseTableDoubleClickAction(/*row_blinking=*/false, session_service_),
      TableDoubleClickAction::kDefaultCommand);
}

}  // namespace
