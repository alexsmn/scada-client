#include "events/audit_entry.h"

#include "model/namespaces.h"
#include "scada/event.h"
#include "scada/standard_node_ids.h"
#include "scada/status.h"

#include <gtest/gtest.h>

namespace events {
namespace {

scada::Event MethodAudit(scada::UInt32 severity = scada::kSeverityNormal) {
  scada::Event event;
  event.event_type_id = scada::NodeId{scada::id::AuditUpdateMethodEventType,
                                      scada::NamespaceIndexes::NS0};
  event.severity = severity;
  return event;
}

scada::Event SessionAudit(scada::UInt32 severity) {
  scada::Event event;
  event.event_type_id = scada::NodeId{scada::id::AuditActivateSessionEventType,
                                      scada::NamespaceIndexes::NS0};
  event.severity = severity;
  return event;
}

// The Server's method name becomes an action an administrator reads, and the
// account it names becomes the object (backlog 1127). Without a catalog the
// translation is the English source, which is what the assertion pins.
TEST(AuditEntry, ASuccessfulCallReadsAsActionObjectApplied) {
  const AuditEntry entry =
      DescribeAuditEvent(MethodAudit(), u"AddIdentity ivanov");
  EXPECT_EQ(entry.action, u"Adding to a role");
  EXPECT_EQ(entry.subject, u"ivanov");
  EXPECT_TRUE(entry.names_role);
  EXPECT_EQ(entry.outcome, AuditOutcome::kApplied);
  EXPECT_EQ(entry.status, u"");
  EXPECT_EQ(AuditOutcomeLabel(entry), u"✓ Applied");
}

// A refused call carries its status after the em dash; it is the result, not
// part of the object.
TEST(AuditEntry, ARefusedCallReadsAsDeniedWithItsStatus) {
  const AuditEntry entry =
      DescribeAuditEvent(MethodAudit(scada::kSeverityWarning),
                         u"RemoveUser kozlov — Bad_UserAccessDenied");
  EXPECT_EQ(entry.action, u"Removing a user");
  EXPECT_EQ(entry.subject, u"kozlov");
  // Called on UserManagement, which says nothing beside the account.
  EXPECT_FALSE(entry.names_role);
  EXPECT_EQ(entry.outcome, AuditOutcome::kDenied);
  EXPECT_EQ(entry.status, u"Bad_UserAccessDenied");
  EXPECT_EQ(AuditOutcomeLabel(entry), u"✕ Denied: Bad_UserAccessDenied");
}

// ChangePassword on an account names no subject: the account is the source
// node, which the journal shows instead.
TEST(AuditEntry, ACallWithNoSubjectLeavesTheObjectToTheSourceNode) {
  const AuditEntry entry = DescribeAuditEvent(
      MethodAudit(), u"ChangePassword — Bad_UserAccessDenied");
  EXPECT_EQ(entry.action, u"Changing a password");
  EXPECT_EQ(entry.subject, u"");
  EXPECT_EQ(entry.outcome, AuditOutcome::kDenied);
}

// A method this Client does not know keeps the Server's word rather than
// leaving the action blank.
TEST(AuditEntry, AnUnknownMethodKeepsTheServersName) {
  const AuditEntry entry =
      DescribeAuditEvent(MethodAudit(), u"UnknownMethod operators");
  EXPECT_EQ(entry.action, u"UnknownMethod");
  EXPECT_EQ(entry.subject, u"operators");
}

// Every method the Server audits has a name of its own.
TEST(AuditEntry, EveryAuditedMethodIsNamed) {
  for (std::u16string_view method :
       {u"AddUser", u"ModifyUser", u"RemoveUser", u"ChangePassword",
        u"VerifyPassword", u"AddRole", u"RemoveRole", u"AddIdentity",
        u"RemoveIdentity"}) {
    // gtest prints both u16 strings; a char message would narrow char16_t,
    // which MSVC rejects (C4244 as an error).
    EXPECT_NE(DescribeAuditEvent(MethodAudit(), method).action, method);
  }
}

// A session activation is prose: the prose is the action, and the severity
// the Server raises a failure with is the outcome.
TEST(AuditEntry, ASessionAuditTakesItsOutcomeFromSeverity) {
  const AuditEntry ok = DescribeAuditEvent(SessionAudit(scada::kSeverityMin),
                                           u"Session activated");
  EXPECT_EQ(ok.action, u"Session activated");
  EXPECT_EQ(ok.outcome, AuditOutcome::kApplied);

  const AuditEntry refused = DescribeAuditEvent(
      SessionAudit(scada::kSeverityWarning), u"Session activation failed");
  EXPECT_EQ(refused.outcome, AuditOutcome::kDenied);
  EXPECT_EQ(refused.status, u"");
  EXPECT_EQ(AuditOutcomeLabel(refused), u"✕ Denied");
}

// UX audit 2026-11-25, item 4.5: a refusal's status reads in the client's
// words when the UI has installed them, not as the server's symbolic name.
TEST(AuditEntry, ADeniedStatusReadsInTheInstalledWords) {
  scada::SetStatusTextProvider([](scada::StatusCode code) -> std::u16string {
    return code == scada::StatusCode::Bad_UserAccessDenied ? u"access denied"
                                                           : u"other";
  });
  AuditEntry entry;
  entry.outcome = AuditOutcome::kDenied;
  entry.status = u"Bad_UserAccessDenied";
  EXPECT_EQ(AuditOutcomeLabel(entry), u"\u2715 Denied: access denied");
  scada::SetStatusTextProvider(nullptr);
}

}  // namespace
}  // namespace events
