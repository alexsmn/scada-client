#pragma once

#include "parameter_form/address_map_row.h"
#include "parameter_form/limit_row.h"
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
class QPushButton;
class QStackedWidget;

// The reshell device-parameter editor — the center workspace of
// docs/product/ui-mockups/screens/config-workbench.html. It presents an
// aui::PropertyModel as a subtabbed form (one tab per top-level property group;
// loose leaf properties fall under a "General" tab), with a Revert / Apply bar
// and a dirty indicator.
//
// Edits are staged (ParameterStaging), not written live: typing into a field
// records a pending edit; Apply replays the staged edits through the model's
// SetValue — the existing write path — and Revert discards them. This is the
// mockup's unsaved-changes model, layered over the same PropertyModel the
// legacy property grid uses, so writes, editors and refresh all go through one
// place.
//
// Opt-in: build this only under the reshell UX theme (see
// MakeDeviceParameterForm).
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

  // Supplies the device's address-map rows. When non-empty, an extra read-only
  // "Address map" tab is shown after the property-group tabs
  // (config-workbench's address-map preview). Passing an empty list removes the
  // tab. Safe to call repeatedly as the async browse streams rows in.
  void SetAddressMap(std::vector<AddressMapRow> rows);

  // Supplies the device's limits rows (analog signals + LoLo/Lo/Hi/HiHi bands).
  // When non-empty, a read-only "Limits" tab is shown after the address-map
  // tab. An empty list removes the tab.
  void SetLimits(std::vector<LimitRow> rows);

 private:
  // One rendered field: the (group, index) it writes to and its editor. The
  // editor is a QLineEdit (TEXT / BUTTON / read-only) or a QComboBox
  // (DROPDOWN), chosen from the model's EditData for that property.
  struct Field {
    scada::aui::PropertyGroup* group = nullptr;
    int index = 0;
    QWidget* editor = nullptr;
  };

  // (Re)reads the model into subtabs + fields. Hooked to the model's change
  // handler so async node loads refresh the form.
  void Rebuild();
  QWidget* BuildSectionPage(scada::aui::PropertyGroup& group,
                            const std::vector<int>& field_indices);
  // Builds the read-only address-map grid page from address_map_.
  QWidget* BuildAddressMapPage();
  // Builds the read-only limits grid page from limits_.
  QWidget* BuildLimitsPage();
  // Builds the editor widget for one property from its EditData (text box,
  // editable dropdown, dialog-button field, or read-only box), seeded with the
  // property's current value. Does not wire the change signal (see
  // ConnectFieldEditor).
  QWidget* CreateFieldEditor(scada::aui::PropertyGroup& group, int index);
  // Wires the field's editor change signal to OnFieldEdited (no-op for a
  // read-only editor).
  void ConnectFieldEditor(const Field& field);
  // Reads / writes the editor's text regardless of editor kind. SetEditorText
  // blocks signals so a programmatic reset is not mistaken for an edit.
  QString EditorText(const Field& field) const;
  void SetEditorText(const Field& field, const QString& text);

  void OnFieldEdited(const Field& field, const QString& text);
  void UpdateDirtyUi();
  void SelectSection(int index);

  ParameterStaging::Key FieldKey(const Field& field) const;

  scada::aui::PropertyModel& model_;
  QString title_;
  ParameterStaging staging_;
  std::vector<AddressMapRow> address_map_;
  std::vector<LimitRow> limits_;

  QLabel* title_label_ = nullptr;
  QHBoxLayout* subtab_layout_ = nullptr;
  std::vector<QPushButton*> subtab_buttons_;
  QStackedWidget* pages_ = nullptr;
  QPushButton* revert_ = nullptr;
  QPushButton* apply_ = nullptr;
  QLabel* dirty_dot_ = nullptr;

  std::vector<Field> fields_;
};

// Builds a DeviceParameterForm. Ownership transfers to the caller.
DeviceParameterForm* MakeDeviceParameterForm(scada::aui::PropertyModel& model,
                                             QString title);
