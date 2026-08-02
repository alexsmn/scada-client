#include "administration/qt/administration_panel.h"

#include "aui/translation.h"

#include <QListWidget>
#include <QVBoxLayout>

namespace {

// The section's command id, carried on its row so activation needs no
// index-to-section lookup that could drift from the list.
constexpr int kCommandIdRole = Qt::UserRole + 1;

}  // namespace

AdministrationPanel::AdministrationPanel(QWidget* parent) : QWidget{parent} {
  auto* layout = new QVBoxLayout{this};
  // The pane is the dock's whole content; the dock supplies the margin.
  layout->setContentsMargins(0, 0, 0, 0);

  list_ = new QListWidget{this};
  list_->setObjectName(QStringLiteral("AdministrationSections"));
  // No frame: the dock already draws one, and a second border reads as a
  // nested panel.
  list_->setFrameShape(QFrame::NoFrame);
  layout->addWidget(list_);

  // itemActivated, not itemClicked: on every platform activation is the
  // deliberate gesture (double-click or Enter), and a single click merely
  // selects. Navigating on selection would move the workspace out from under
  // an operator arrowing through the list.
  connect(list_, &QListWidget::itemActivated, this,
          [this](QListWidgetItem* item) {
            if (!item) {
              return;
            }
            const int command_id = item->data(kCommandIdRole).toInt();
            if (command_id != 0) {
              Q_EMIT SectionActivated(static_cast<unsigned>(command_id));
            }
          });
}

AdministrationPanel::~AdministrationPanel() = default;

void AdministrationPanel::ShowSections(
    std::span<const AdministrationSection> sections) {
  sections_.assign(sections.begin(), sections.end());

  list_->clear();
  for (const AdministrationSection& section : sections_) {
    auto* item = new QListWidgetItem{
        QString::fromStdU16String(Translate(section.label)), list_};
    item->setData(kCommandIdRole, static_cast<int>(section.command_id));
  }
}
