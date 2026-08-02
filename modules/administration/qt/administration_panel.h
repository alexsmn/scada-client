#pragma once

#include "administration/administration_sections.h"

#include <QWidget>

#include <span>
#include <vector>

class QListWidget;

// The Administration explorer pane: the section list an administrator
// navigates the admin surfaces from (the left region of
// `docs/product/ui-mockups/screens/users-admin.html`).
//
// Deliberately a plain list rather than a tree — the sections are a flat set of
// destinations, and a tree would imply a containment that does not exist.
// Native chrome throughout: a stock `QListWidget` styled by the platform, per
// `docs/client/ux/README.md`.
class AdministrationPanel : public QWidget {
  Q_OBJECT

 public:
  explicit AdministrationPanel(QWidget* parent = nullptr);
  ~AdministrationPanel() override;

  // Replaces the visible sections. Pass only sections the shell can actually
  // open (see `ResolvableAdministrationSections`) — this widget renders what it
  // is given and does not second-guess availability.
  void ShowSections(std::span<const AdministrationSection> sections);

  // Sections currently shown, in display order. For tests.
  const std::vector<AdministrationSection>& sections() const {
    return sections_;
  }

  // A section was chosen (activated, not merely selected — a single click
  // should not navigate away from what the operator is looking at).
 Q_SIGNALS:
  void SectionActivated(unsigned command_id);

 private:
  QListWidget* list_ = nullptr;
  std::vector<AdministrationSection> sections_;
};
