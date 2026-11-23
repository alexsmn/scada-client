#pragma once

#include <string>
#include <string_view>

namespace scada {
class SessionService;
}

// Whether the signed-in account may switch control confirmation off.
//
// Only an administrator may (decided 2026-11-23, backlog 1118): the review is
// the second half of the two-stage confirm principle §7 asks for
// (docs/client/ux/principles.md), and an operator who could untick it in
// Settings could send commands to the field with no review at all. An
// administrator keeps the switch for commissioning and test benches. The
// Client calls the Configure right "administrator" everywhere else too
// (`WIN_REQUIRES_ADMIN`, the change-password command).
bool CanSwitchOffControlConfirmation(
    const scada::SessionService& session_service);

// Whether a control command is reviewed before it is sent: the profile's
// switch is honoured only when `may_switch_off` — a stored "off" from an
// administrator's session never reaches an operator's.
bool IsControlConfirmationRequired(bool profile_confirmation,
                                   bool may_switch_off);

// The disabled reason the «Control Confirmation» setting shows to an account
// that may not switch it off, as an untranslated English key for Translate().
std::string_view ControlConfirmationAdminOnlyText();

// What the operator reviews before a control command is sent. Every field is
// already formatted for display; `WriteModel::GetControlReview` builds it and
// the Qt review dialog only lays it out, so the shipped wording is testable
// without a widget.
struct ControlReview {
  // The window title, «Control».
  std::u16string title;
  // The object the command goes to, by its display name.
  std::u16string object;
  // What the point reads now, formatted as the Control dialog shows it —
  // including the «?» of an invalid reading and the letters of a stale one.
  std::u16string present;
  // What the command makes it.
  std::u16string command;
  // Who the command is logged against.
  std::u16string operator_name;
  // When the review was opened.
  std::u16string time;
  // Which stage of the command this is: a one-stage command, or the operate
  // half of a select-before-operate command.
  std::u16string stage;
  // The irreversibility warning.
  std::u16string warning;
};
