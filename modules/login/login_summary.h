#pragma once

#include <string>
#include <string_view>

// The sign-in card's "what am I about to connect to" line, composed from the
// dialog's own selection (see the reshell mockup
// client/docs/ui-mockups/screens/login.html, "System preview").
//
// Only what the dialog genuinely knows before authenticating is shown — the
// chosen backend and server. Reachability, round-trip and plant status from
// the mockup need either a pre-auth probe or a session, neither of which
// exists at this point (see the backlog's 3.1 scope note).
//
// Pure so it can be unit-tested without a QApplication.

// Composes the summary. Returns:
//   "<backend> · <server>" when both are known,
//   the known one alone when only one is,
//   an empty string when neither is — the caller then hides the line rather
//   than showing a stray separator.
std::u16string LoginConnectionSummary(std::u16string_view backend,
                                      std::u16string_view server);
