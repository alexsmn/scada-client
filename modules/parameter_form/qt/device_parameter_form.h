#pragma once

#include "parameter_form/parameter_staging.h"

#include <QString>
#include <QWidget>

#include <vector>

namespace scada::aui {
class PropertyModel;
class PropertyGroup;
}  // namespace scada::aui
class QHBoxLayout;
class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;

// The reshell device-parameter editor — the center workspace of
// client/docs/ui-mockups/screens/config-workbench.html. It presents an
// aui::PropertyModel as a subtabbed form (one tab per top-level property group;
// loose leaf properties fall under a "General" tab), with a Revert / Apply bar
// and a dirty indicator.
//
// Edits are staged (ParameterStaging), not written live: typing into a field
// records a pending edit; Apply replays the staged edits through the model's
// SetValue — the existing write path — and Revert discards them. This is the
// mockup's unsaved-changes model, layered over the same PropertyModel the legacy
// property grid uses, so writes, editors and refresh all go through one place.
//
// Opt-in: build this only under the reshell UX theme (see
// MakeDeviceParameterForm); the legacy look keeps the property grid.
class DeviceParameterForm : public QWidget {
  Q_OBJECT

 public:
  DeviceParameterForm(scada::aui::PropertyModel& model,
                      QString title,
                      QWidget* parent = nullptr);
  ~DeviceParameterForm() override;

  // True while there are staged (unwritten) edits.
  bool dirty() const;

  // Writes every staged edit through the model, then clears the buffer.
  void Apply();
  // Discards every staged edit, restoring fields to the model's live values.
  void Revert();

 private:
  // One rendered field: the (group, index) it writes to and its editor.
  struct Field {
    scada::aui::PropertyGroup* group = nullptr;
    int index = 0;
    QLineEdit* editor = nullptr;
  };

  // (Re)reads the model into subtabs + fields. Hooked to the model's change
  // handler so async node loads refresh the form.
  void Rebuild();
  QWidget* BuildSectionPage(scada::aui::PropertyGroup& group,
                            const std::vector<int>& field_indices);
  void OnFieldEdited(const Field& field, const QString& text);
  void UpdateDirtyUi();
  void SelectSection(int index);

  ParameterStaging::Key FieldKey(const Field& field) const;

  scada::aui::PropertyModel& model_;
  QString title_;
  ParameterStaging staging_;

  QLabel* title_label_ = nullptr;
  QHBoxLayout* subtab_layout_ = nullptr;
  std::vector<QPushButton*> subtab_buttons_;
  QStackedWidget* pages_ = nullptr;
  QPushButton* revert_ = nullptr;
  QPushButton* apply_ = nullptr;
  QLabel* dirty_dot_ = nullptr;

  std::vector<Field> fields_;
};

// Builds a DeviceParameterForm under the reshell UX theme
// (scada::aui::GetSeverityTheme() != SeverityTheme::kLegacy); returns nullptr in
// the legacy look so the host keeps the property grid. Ownership transfers to
// the caller.
DeviceParameterForm* MakeDeviceParameterForm(scada::aui::PropertyModel& model,
                                             QString title);
