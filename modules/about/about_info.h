#pragma once

#include <string>
#include <string_view>

// Everything the About dialog states, as data.
//
// The dialog's job is to be QUOTED BACK — into a ticket, a chat window, an
// email to the integrator — so the clipboard payload is the primary artefact
// rather than a secondary rendering of the screen. It is assembled here, free
// of Qt, so it can be tested without a QApplication, and so the screenshot
// generator can hand the dialog a fixed one.
//
// The web client's About carries the same fields in the same order
// (`web/apps/app/src/features/about/about-details.ts`) and produces a payload
// of the same shape, so two reports of one fault can be read side by side. The
// shared screen both realms draw is
// `docs/product/ui-mockups/screens/about.html`.
//
// **Every field but `product` and `version` may be empty, and empty means the
// row is not drawn.** A row nobody can fill is absent, never blank: reached
// from the login window there is no session at all, and the dialog must never
// state a server it is not talking to. `build_commit` is the single
// exception — the dialog says "not recorded" there, because silence would read
// as "this build has no identity" rather than "this build was made outside a
// git checkout".
struct AboutInfo {
  // The application's own name, as the window manager and the menu bar use it.
  std::u16string product;
  // Marketing version, e.g. `2.6.0`.
  std::u16string version;
  // Abbreviated commit. Empty when the build was made outside a checkout.
  std::u16string build_commit;
  // The COMMIT's date, ISO-8601 — never the build clock, so two builds of one
  // commit describe themselves identically. Empty whenever `build_commit` is.
  std::u16string build_date;
  // What this binary runs on: Qt version, OS and architecture.
  std::u16string runtime;
  // The endpoint this client is pointed at.
  std::u16string server;
  // The signed-in account. Empty for an anonymous session.
  std::u16string user;
  // False when the dialog was opened before there was anything to describe —
  // from the login window's Help menu. It is what makes the three session rows
  // absent rather than blank, and it is a flag rather than an empty `server`
  // because a session whose endpoint is unknown is still a session.
  bool has_session = false;
  bool connected = false;
};

// The clipboard payload: one plain-text block, fixed shape, **fixed English
// labels**.
//
// The labels are deliberately not translated, on the same reasoning that has
// the CSV export write ISO-8601 timestamps into a file while the journal
// renders local time. This block is addressed to whoever receives the ticket —
// often an integrator on the other side of a contract — and a fixed shape is
// one a person can scan and a script can parse. What is localised is the
// screen.
//
// `reported_at` is passed in rather than read from the clock, so the formatter
// is deterministic under test and produces a stable screenshot.
//
// Note what `AboutInfo` deliberately does NOT hold: any word a reader reads.
// `connected`, `anonymous` and `not recorded` are produced here in English and
// by the dialog in the operator's language, from the same two flags — so the
// screen and the payload cannot come to disagree, which they could if the
// struct carried one pre-rendered string for both to show.
std::u16string FormatAboutDetails(const AboutInfo& info,
                                  std::u16string_view reported_at);
