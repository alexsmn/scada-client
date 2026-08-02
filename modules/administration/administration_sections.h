#pragma once

#include <span>
#include <vector>

// The Administration explorer's section list — the left pane of the admin
// surface (`docs/product/ui-mockups/screens/users-admin.html`).
//
// Qt-free on purpose, like `main_window/pane_modes.h`: the vocabulary is the
// product's, so the Wt shell and the web client can describe the same sections
// in the same words.

// One section. Activating it opens the workspace view registered under
// `command_id` — the section list navigates, it never renders the surface
// itself.
struct AdministrationSection {
  // The global command that opens the section's view. It is also the section's
  // identity: a section with no command is not a section, it is a plan.
  unsigned command_id = 0;
  // English source string; call sites pass it through Translate().
  const char* label = nullptr;
};

// Every section the client knows how to open, in display order.
//
// This is the CANDIDATE list, not what an operator sees. A section reaches the
// pane only when the shell can actually resolve its command — see
// `ResolvableAdministrationSections`. That is what keeps the pane from
// offering a door that opens onto nothing as the remaining admin surfaces
// (roles, password policy, the audit log) are built.
std::span<const AdministrationSection> GetAdministrationSections();

// The subset of `GetAdministrationSections()` whose command `is_available`
// accepts, preserving order.
//
// Availability is the shell's own answer (`ControllerDelegate::
// ResolveViewCommand`), so it already accounts for both "this view does not
// exist in this build" and "this session may not open it" — the admin gate on
// `WIN_REQUIRES_ADMIN` views is enforced by the same resolution. The pane
// therefore never has to duplicate the permission rule, and cannot drift from
// it.
std::vector<AdministrationSection> ResolvableAdministrationSections(
    std::span<const AdministrationSection> candidates,
    const auto& is_available)
  requires requires(unsigned command_id) {
    { is_available(command_id) } -> std::convertible_to<bool>;
  }
{
  std::vector<AdministrationSection> sections;
  for (const AdministrationSection& section : candidates) {
    if (is_available(section.command_id)) {
      sections.push_back(section);
    }
  }
  return sections;
}
