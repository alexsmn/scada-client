#include "user_access/password_policy.h"

#include "base/utf_convert.h"
#include "model/namespaces.h"
#include "scada/extension_object.h"
#include "scada/range_encoding.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "scada/basic_types.h"
#include "scada/standard_node_ids.h"
#include "scada/variant.h"

#include <algorithm>

namespace {

const scada::NodeId kUserManagement{
    scada::id::Server_ServerConfiguration_UserManagement,
    scada::NamespaceIndexes::NS0};
const scada::NodeId kPasswordLength{scada::id::UserManagement_PasswordLength,
                                    scada::NamespaceIndexes::NS0};
const scada::NodeId kPasswordOptions{scada::id::UserManagement_PasswordOptions,
                                     scada::NamespaceIndexes::NS0};
const scada::NodeId kPasswordRestrictions{
    scada::id::UserManagement_PasswordRestrictions,
    scada::NamespaceIndexes::NS0};

bool Has(scada::PasswordOptions options, scada::PasswordOptions bit) {
  return scada::HasPasswordOption(options, bit);
}

// True when any character of `password` satisfies `predicate`.
bool AnyOf(const std::u16string& password, bool (*predicate)(char16_t)) {
  return std::ranges::any_of(password, predicate);
}

}  // namespace

Awaitable<std::optional<PasswordPolicy>> ReadPasswordPolicy(
    AnyExecutor executor,
    NodeService& node_service) {
  NodeRef user_management = node_service.GetNode(kUserManagement);
  if (!user_management) {
    co_return std::nullopt;
  }
  co_await user_management.Fetch(NodeFetchStatus::NodeAndChildren);

  PasswordPolicy policy;
  bool read_anything = false;

  if (NodeRef length = node_service.GetNode(kPasswordLength)) {
    co_await length.Fetch(NodeFetchStatus::NodeOnly);
    // PasswordLength is a Range, carried as an ExtensionObject (Part 18
    // §5.2.2).
    scada::ExtensionObject object;
    if (length.value().get(object)) {
      if (auto range = scada::DecodeRangeObject(object)) {
        policy.min_length = range->low;
        policy.max_length = range->high;
        read_anything = true;
      }
    }
  }

  if (NodeRef options = node_service.GetNode(kPasswordOptions)) {
    co_await options.Fetch(NodeFetchStatus::NodeOnly);
    scada::UInt32 raw = 0;
    if (options.value().get(raw)) {
      policy.options = static_cast<scada::PasswordOptions>(raw);
      read_anything = true;
    }
  }

  if (NodeRef restrictions = node_service.GetNode(kPasswordRestrictions)) {
    co_await restrictions.Fetch(NodeFetchStatus::NodeOnly);
    scada::LocalizedText text;
    if (restrictions.value().get(text)) {
      policy.restrictions = ToString16(text);
      read_anything = true;
    }
  }

  // Not one property came back: the object exists but nothing was served, so
  // report unknown rather than an unconstrained policy. The two must not look
  // alike — one means "any password is fine", the other means "we do not know
  // what is fine".
  if (!read_anything) {
    co_return std::nullopt;
  }
  co_return policy;
}

std::vector<PasswordRequirement> PasswordRequirementsFor(
    scada::PasswordOptions options) {
  return {
      {"Upper-case letter",
       Has(options, scada::PasswordOptions::kRequiresUpperCaseCharacters)},
      {"Lower-case letter",
       Has(options, scada::PasswordOptions::kRequiresLowerCaseCharacters)},
      {"Digit", Has(options, scada::PasswordOptions::kRequiresDigitCharacters)},
      {"Special character",
       Has(options, scada::PasswordOptions::kRequiresSpecialCharacters)},
  };
}

const char* PasswordPolicyViolation(const PasswordPolicy& policy,
                                    const std::u16string& password) {
  // A non-positive bound is "unconstrained", which is what a server with no
  // length policy publishes — not a zero-length requirement.
  if (policy.min_length > 0 &&
      static_cast<double>(password.size()) < policy.min_length) {
    return "The password is too short";
  }
  if (policy.max_length > 0 &&
      static_cast<double>(password.size()) > policy.max_length) {
    return "The password is too long";
  }
  if (Has(policy.options,
          scada::PasswordOptions::kRequiresUpperCaseCharacters) &&
      !AnyOf(password, [](char16_t c) { return c >= u'A' && c <= u'Z'; })) {
    return "The password needs an upper-case letter";
  }
  if (Has(policy.options,
          scada::PasswordOptions::kRequiresLowerCaseCharacters) &&
      !AnyOf(password, [](char16_t c) { return c >= u'a' && c <= u'z'; })) {
    return "The password needs a lower-case letter";
  }
  if (Has(policy.options, scada::PasswordOptions::kRequiresDigitCharacters) &&
      !AnyOf(password, [](char16_t c) { return c >= u'0' && c <= u'9'; })) {
    return "The password needs a digit";
  }
  if (Has(policy.options,
          scada::PasswordOptions::kRequiresSpecialCharacters) &&
      !AnyOf(password, [](char16_t c) {
        // Anything that is not a letter or a digit. Deliberately broader than
        // an ASCII punctuation list: the accounts here carry Cyrillic names,
        // and a rule that rejected a non-ASCII symbol the SERVER accepts would
        // block a password that is in fact valid.
        return !((c >= u'A' && c <= u'Z') || (c >= u'a' && c <= u'z') ||
                 (c >= u'0' && c <= u'9'));
      })) {
    return "The password needs a special character";
  }
  return nullptr;
}
