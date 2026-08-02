#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "scada/authorization.h"

#include <optional>
#include <string>
#include <vector>

class NodeService;

// The password policy this server publishes on the UserManagement object
// (OPC UA Part 18 §5.2.2): what a new password must satisfy, and which
// account-management capabilities the server actually implements.
//
// The client reads it rather than hard-coding rules, so the dialog that asks
// for a password and the server that validates it cannot disagree — and so an
// operator is told the rule up front instead of discovering it from a
// rejection.
struct PasswordPolicy {
  // PasswordLength, as a Range. A non-positive bound means unconstrained,
  // which is what a server with no length policy publishes.
  double min_length = 0;
  double max_length = 0;

  // The PasswordOptionsMask. Its kRequires* bits are the policy; its kSupport*
  // bits describe which account-management operations the server implements,
  // and are what a client greys an affordance out on rather than issuing a
  // call the server would refuse.
  scada::PasswordOptions options = scada::PasswordOptions::kNone;

  // The server's own human-readable statement of the policy. Shown verbatim:
  // it is the deployment's wording, and paraphrasing it in the client would
  // put two descriptions of one rule out of step.
  std::u16string restrictions;
};

// Reads the policy. Returns nullopt when the UserManagement object could not
// be read, which the caller must not render as "no policy" — an unconstrained
// server and an unreadable one look identical otherwise, and the difference
// decides whether a rejected password is the operator's fault.
Awaitable<std::optional<PasswordPolicy>> ReadPasswordPolicy(
    AnyExecutor executor,
    NodeService& node_service);

// One rendered policy line: a requirement and whether this server imposes it.
struct PasswordRequirement {
  // English source string for Translate().
  const char* label;
  bool required;
};

// The kRequires* breakdown, in display order.
std::vector<PasswordRequirement> PasswordRequirementsFor(
    scada::PasswordOptions options);

// Whether `password` satisfies `policy`, as the reason it does not — nullptr
// when it does.
//
// This is a COURTESY check, never the enforcement: the server validates every
// password itself (CheckPasswordPolicy), and a client that believed its own
// verdict would let a deployment-specific rule through. It exists so the
// operator learns of a problem before the round trip.
const char* PasswordPolicyViolation(const PasswordPolicy& policy,
                                    const std::u16string& password);
