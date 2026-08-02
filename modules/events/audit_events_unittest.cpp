#include "events/audit_events.h"

#include "model/devices_node_ids.h"
#include "model/namespaces.h"
#include "scada/standard_node_ids.h"

#include <gtest/gtest.h>

namespace {

scada::NodeId Ns0(scada::NumericId id) {
  return scada::NodeId{id, scada::NamespaceIndexes::NS0};
}

// Every audit type the historian stores must be recognised here. If it is not,
// the events are written and shown nowhere — an audit log that omits entries
// while reading as complete, which is worse than no audit log at all.
TEST(AuditEvents, RecognisesEveryStoredAuditType) {
  EXPECT_TRUE(IsAuditEventType(Ns0(scada::id::AuditEventType)));
  EXPECT_TRUE(IsAuditEventType(Ns0(scada::id::AuditActivateSessionEventType)));
  EXPECT_TRUE(IsAuditEventType(Ns0(scada::id::AuditUpdateMethodEventType)));
}

// The counterpart: ordinary process events must not leak into the trail, or
// the audit log stops being an audit log.
TEST(AuditEvents, RejectsOrdinaryEventTypes) {
  EXPECT_FALSE(IsAuditEventType(Ns0(scada::id::SystemEventType)));
  EXPECT_FALSE(IsAuditEventType(Ns0(scada::id::BaseEventType)));
  EXPECT_FALSE(IsAuditEventType(scada::devices::id::DeviceWatchEventType));
}

// A vendor type is not part of the standard trail even if its numeric id
// happens to collide with an NS0 audit id — the namespace is what decides.
TEST(AuditEvents, ANonNs0TypeIsNeverAudit) {
  EXPECT_FALSE(IsAuditEventType(scada::NodeId{
      scada::id::AuditUpdateMethodEventType, scada::NamespaceIndexes::SCADA}));
}

// A string node id cannot be one of the standard numeric types, and must not
// be mistaken for one.
TEST(AuditEvents, ANonNumericIdIsNeverAudit) {
  EXPECT_FALSE(IsAuditEventType(
      scada::NodeId{std::string{"AuditUpdateMethodEventType"},
                    scada::NamespaceIndexes::NS0}));
}

}  // namespace
