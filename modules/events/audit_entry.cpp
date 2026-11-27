#include "events/audit_entry.h"

#include "aui/translation.h"
#include "base/utf_convert.h"
#include "model/namespaces.h"
#include "scada/event.h"
#include "scada/standard_node_ids.h"
#include "scada/status.h"

#include <array>
#include <optional>
#include <string_view>
#include <utility>

namespace events {

namespace {

// The method names the Server writes, untranslated, at the head of a method
// audit (the `method_name` choices in the security module's user_management,
// role_set and security io managers), each with the English source of its
// action name. The Russian is in app/qt/client_ru.ts.
constexpr std::array<std::pair<std::u16string_view, std::string_view>, 9>
    kAuditedMethods = {{
        {u"AddUser", "Adding a user"},
        {u"ModifyUser", "Modifying a user"},
        {u"RemoveUser", "Removing a user"},
        {u"ChangePassword", "Changing a password"},
        {u"VerifyPassword", "Verifying a password"},
        {u"AddRole", "Adding a role"},
        {u"RemoveRole", "Removing a role"},
        {u"AddIdentity", "Adding to a role"},
        {u"RemoveIdentity", "Removing from a role"},
    }};

// What MakeMethodAuditEvent puts between the call and a bad status.
constexpr std::u16string_view kStatusSeparator = u" — ";

std::u16string ActionName(std::u16string_view method) {
  for (const auto& [name, source] : kAuditedMethods) {
    if (name == method)
      return Translate(source);
  }
  // A method this Client predates: the Server's own word beats a blank cell.
  return std::u16string{method};
}

bool IsMethodAudit(const scada::NodeId& event_type_id) {
  return event_type_id.namespace_index() == scada::NamespaceIndexes::NS0 &&
         event_type_id.type() == scada::NodeIdType::Numeric &&
         event_type_id.numeric_id() == scada::id::AuditUpdateMethodEventType;
}

}  // namespace

AuditEntry DescribeAuditEvent(const scada::Event& event,
                              std::u16string_view message) {
  AuditEntry entry;

  if (!IsMethodAudit(event.event_type_id)) {
    // Free-text prose (a session activation): it is the action. The Server
    // raises a failed one above Normal (MakeSessionAuditEvent in
    // scada-server-framework/modules/opcua/opcua_module.cpp).
    entry.action = std::u16string{message};
    entry.outcome = event.severity > scada::kSeverityNormal
                        ? AuditOutcome::kDenied
                        : AuditOutcome::kApplied;
    return entry;
  }

  std::u16string_view call = message;
  if (const size_t separator = call.find(kStatusSeparator);
      separator != std::u16string_view::npos) {
    entry.status =
        std::u16string{call.substr(separator + kStatusSeparator.size())};
    call = call.substr(0, separator);
  }
  entry.outcome =
      entry.status.empty() ? AuditOutcome::kApplied : AuditOutcome::kDenied;

  const size_t space = call.find(u' ');
  const std::u16string_view method = call.substr(0, space);
  entry.action = ActionName(method);
  // RoleType's own methods (OPC UA Part 18 §4.4.5 AddIdentity, §4.4.6
  // RemoveIdentity,
  // https://reference.opcfoundation.org/Core/Part18/v105/docs/4.4) are called
  // on the Role.
  entry.names_role = method == u"AddIdentity" || method == u"RemoveIdentity";
  if (space != std::u16string_view::npos)
    entry.subject = std::u16string{call.substr(space + 1)};
  return entry;
}

std::u16string AuditOutcomeLabel(const AuditEntry& entry) {
  switch (entry.outcome) {
    case AuditOutcome::kApplied:
      return u"✓ " + Translate("Applied");
    case AuditOutcome::kDenied: {
      std::u16string label = u"✕ " + Translate("Denied");
      // The server names the status symbolically («Bad_UserAccessDenied»);
      // say it in the client's words when the code is known (UX audit
      // 2026-11-25, item 4.5), and keep the name when it is not.
      if (!entry.status.empty()) {
        const std::optional<scada::StatusCode> code = StatusCodeFromName(
            UtfConvert<char>(std::u16string_view{entry.status}));
        label += u": " + (code ? ToString16(*code) : entry.status);
      }
      return label;
    }
    case AuditOutcome::kUnknown:
      break;
  }
  return {};
}

}  // namespace events
