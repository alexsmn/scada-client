#include "parameter_form/qt/device_parameter_form.h"

#include "aui/models/property_model.h"
#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHeaderView>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStyle>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>

#include <memory>
#include <string_view>
#include <utility>

namespace {

using scada::aui::PropertyGroup;

// The design tokens for the active reshell theme. The form is only built under a
// token theme (the factory gates on it), so the legacy fallback is harmless.
const scada::aui::ThemeTokens& FormTokens() {
  scada::aui::Theme theme = scada::aui::Theme::kDark;
  switch (scada::aui::GetSeverityTheme()) {
    case scada::aui::SeverityTheme::kLight:
      theme = scada::aui::Theme::kLight;
      break;
    case scada::aui::SeverityTheme::kHighContrast:
      theme = scada::aui::Theme::kHighContrast;
      break;
    default:
      break;
  }
  return scada::aui::GetThemeTokens(theme);
}

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

bool IsSection(PropertyGroup& group, int index) {
  return group.GetSubgroup(index) != nullptr;
}

}  // namespace

DeviceParameterForm::DeviceParameterForm(scada::aui::PropertyModel& model,
                                         QString title,
                                         QWidget* parent)
    : QWidget{parent}, model_{model}, title_{std::move(title)} {
  const scada::aui::ThemeTokens& tokens = FormTokens();
  setObjectName(QStringLiteral("deviceParameterForm"));
  setStyleSheet(QStringLiteral("#deviceParameterForm{background:%1;}")
                    .arg(tokens.bg.name()));

  auto* root = new QVBoxLayout{this};
  root->setContentsMargins(0, 0, 0, 0);
  root->setSpacing(0);

  // Form bar: title + dirty dot + subtabs + Revert / Apply.
  auto* bar = new QWidget;
  bar->setObjectName(QStringLiteral("parameterFormBar"));
  bar->setStyleSheet(
      QStringLiteral("#parameterFormBar{background:%1;border-bottom:1px solid "
                     "%2;}")
          .arg(tokens.bg_elevated.name(), tokens.border.name()));
  auto* bar_layout = new QHBoxLayout{bar};
  bar_layout->setContentsMargins(14, 6, 14, 6);
  bar_layout->setSpacing(8);

  title_label_ = new QLabel{title_};
  title_label_->setStyleSheet(
      QStringLiteral("color:%1;font-weight:600;").arg(tokens.fg.name()));
  bar_layout->addWidget(title_label_);

  dirty_dot_ = new QLabel;
  dirty_dot_->setObjectName(QStringLiteral("parameterDirtyDot"));
  dirty_dot_->setFixedSize(8, 8);
  dirty_dot_->setStyleSheet(
      QStringLiteral("#parameterDirtyDot{border-radius:4px;background:%1;}")
          .arg(tokens.uncertain.name()));
  dirty_dot_->setToolTip(Tr("Unsaved changes"));
  bar_layout->addWidget(dirty_dot_);

  subtab_layout_ = new QHBoxLayout;
  subtab_layout_->setContentsMargins(8, 0, 0, 0);
  subtab_layout_->setSpacing(2);
  bar_layout->addLayout(subtab_layout_);

  bar_layout->addStretch(1);

  revert_ = new QPushButton{Tr("Revert")};
  revert_->setObjectName(QStringLiteral("parameterRevert"));
  revert_->setStyleSheet(
      QStringLiteral("QPushButton{background:%1;color:%2;border:1px solid %3;"
                     "border-radius:6px;padding:5px 12px;}"
                     "QPushButton:disabled{color:%4;}")
          .arg(tokens.surface_muted.name(), tokens.fg.name(),
               tokens.border_strong.name(), tokens.fg_subtle.name()));
  connect(revert_, &QPushButton::clicked, this, [this] { Revert(); });
  bar_layout->addWidget(revert_);

  apply_ = new QPushButton{Tr("Apply")};
  apply_->setObjectName(QStringLiteral("parameterApply"));
  apply_->setStyleSheet(
      QStringLiteral("QPushButton{background:%1;color:%2;border:none;"
                     "border-radius:6px;padding:5px 14px;font-weight:600;}"
                     "QPushButton:disabled{background:%3;color:%4;}")
          .arg(tokens.accent.name(), tokens.accent_fg.name(),
               tokens.surface_muted.name(), tokens.fg_subtle.name()));
  connect(apply_, &QPushButton::clicked, this, [this] { Apply(); });
  bar_layout->addWidget(apply_);

  root->addWidget(bar);

  pages_ = new QStackedWidget;
  root->addWidget(pages_, 1);

  model_.model_changed_handler = [this] { Rebuild(); };
  Rebuild();
}

DeviceParameterForm::~DeviceParameterForm() {
  // Drop the handler so a late model change cannot call into a destroyed form.
  model_.model_changed_handler = nullptr;
}

ParameterStaging::Key DeviceParameterForm::FieldKey(const Field& field) const {
  return {static_cast<const void*>(field.group), field.index};
}

bool DeviceParameterForm::dirty() const {
  return staging_.dirty();
}

QWidget* DeviceParameterForm::BuildSectionPage(
    PropertyGroup& group,
    const std::vector<int>& field_indices) {
  const scada::aui::ThemeTokens& tokens = FormTokens();

  auto* page = new QWidget;
  auto* outer = new QVBoxLayout{page};
  outer->setContentsMargins(18, 16, 18, 16);

  auto* form = new QFormLayout;
  form->setLabelAlignment(Qt::AlignLeft | Qt::AlignVCenter);
  form->setHorizontalSpacing(16);
  form->setVerticalSpacing(10);

  for (int index : field_indices) {
    auto* label = new QLabel{QString::fromStdU16String(group.GetName(index))};
    label->setStyleSheet(
        QStringLiteral("color:%1;").arg(tokens.fg_muted.name()));

    // The editor kind (text box / dropdown / dialog-button / read-only) comes
    // from the property's EditData, seeded with its current value.
    QWidget* editor = CreateFieldEditor(group, index);
    Field field{&group, index, editor};
    // A pending edit for this field shows the staged value; blocked so it is not
    // re-registered as an edit.
    if (const std::u16string* staged = staging_.Get(FieldKey(field)))
      SetEditorText(field, QString::fromStdU16String(*staged));
    ConnectFieldEditor(field);
    fields_.push_back(field);
    form->addRow(label, editor);
  }

  outer->addLayout(form);
  outer->addStretch(1);

  auto* scroll = new QScrollArea;
  scroll->setFrameShape(QFrame::NoFrame);
  scroll->setWidgetResizable(true);
  scroll->setWidget(page);
  return scroll;
}

QWidget* DeviceParameterForm::CreateFieldEditor(PropertyGroup& group,
                                                int index) {
  const scada::aui::ThemeTokens& tokens = FormTokens();
  const QString value = QString::fromStdU16String(group.GetValue(index));
  const QString box_style =
      QStringLiteral("background:%1;color:%2;border:1px solid %3;"
                     "border-radius:6px;padding:5px 10px;")
          .arg(tokens.bg_elevated.name(), tokens.fg.name(),
               tokens.border_strong.name());

  const scada::aui::EditData edit_data = group.GetEditData(index);
  switch (edit_data.editor_type) {
    case scada::aui::EditData::EditorType::DROPDOWN: {
      auto* combo = new QComboBox;
      combo->setEditable(true);
      combo->setInsertPolicy(QComboBox::NoInsert);
      combo->setStyleSheet(
          QStringLiteral("QComboBox{%1}").arg(box_style));
      if (edit_data.async_choice_handler) {
        // Populate asynchronously (mirrors the legacy grid's delegate): the
        // handler streams choices in behind a trailing "Loading…" row. Guard
        // against the combo being destroyed before the callback fires.
        combo->addItem(Tr("Loading…"));
        auto canceled = std::make_shared<bool>(false);
        connect(combo, &QObject::destroyed, [canceled] { *canceled = true; });
        edit_data.async_choice_handler(
            [combo, canceled](const std::vector<std::u16string>& choices,
                              bool last) {
              if (*canceled)
                return;
              for (const std::u16string& choice : choices) {
                combo->insertItem(combo->count() - 1,
                                  QString::fromStdU16String(choice));
              }
              if (last)
                combo->removeItem(combo->count() - 1);
            });
      } else {
        for (const std::u16string& choice : edit_data.choices)
          combo->addItem(QString::fromStdU16String(choice));
      }
      combo->setCurrentText(value);
      return combo;
    }

    case scada::aui::EditData::EditorType::BUTTON: {
      auto* line = new QLineEdit;
      line->setStyleSheet(QStringLiteral("QLineEdit{%1}").arg(box_style));
      line->setText(value);
      QAction* action = line->addAction(
          QApplication::style()->standardIcon(
              QStyle::SP_FileDialogDetailedView),
          QLineEdit::TrailingPosition);
      PropertyGroup* group_ptr = &group;
      connect(action, &QAction::triggered, this,
              [group_ptr, index] { group_ptr->HandleEditButton(index); });
      return line;
    }

    case scada::aui::EditData::EditorType::NONE: {
      // Display-only: a read-only, muted box. Never staged.
      auto* line = new QLineEdit;
      line->setReadOnly(true);
      line->setStyleSheet(
          QStringLiteral("QLineEdit{background:%1;color:%2;border:1px solid %3;"
                         "border-radius:6px;padding:5px 10px;}")
              .arg(tokens.surface_muted.name(), tokens.fg_subtle.name(),
                   tokens.border.name()));
      line->setText(value);
      return line;
    }

    case scada::aui::EditData::EditorType::TEXT:
    default: {
      auto* line = new QLineEdit;
      line->setStyleSheet(QStringLiteral("QLineEdit{%1}").arg(box_style));
      line->setText(value);
      return line;
    }
  }
}

void DeviceParameterForm::ConnectFieldEditor(const Field& field) {
  if (auto* line = qobject_cast<QLineEdit*>(field.editor)) {
    if (line->isReadOnly())
      return;  // NONE editor: display only, never staged.
    connect(line, &QLineEdit::textChanged, this,
            [this, field](const QString& text) { OnFieldEdited(field, text); });
  } else if (auto* combo = qobject_cast<QComboBox*>(field.editor)) {
    // editTextChanged fires on both typing and selecting an item (selection
    // updates the edit text), but not on the async insert/remove of choices —
    // so populating the dropdown does not register as an edit.
    connect(combo, &QComboBox::editTextChanged, this,
            [this, field](const QString& text) { OnFieldEdited(field, text); });
  }
}

QString DeviceParameterForm::EditorText(const Field& field) const {
  if (auto* line = qobject_cast<QLineEdit*>(field.editor))
    return line->text();
  if (auto* combo = qobject_cast<QComboBox*>(field.editor))
    return combo->currentText();
  return {};
}

void DeviceParameterForm::SetEditorText(const Field& field,
                                        const QString& text) {
  QSignalBlocker blocker{field.editor};
  if (auto* line = qobject_cast<QLineEdit*>(field.editor))
    line->setText(text);
  else if (auto* combo = qobject_cast<QComboBox*>(field.editor))
    combo->setCurrentText(text);
}

void DeviceParameterForm::Rebuild() {
  const scada::aui::ThemeTokens& tokens = FormTokens();

  fields_.clear();
  for (QPushButton* button : subtab_buttons_)
    button->deleteLater();
  subtab_buttons_.clear();
  while (pages_->count() > 0) {
    QWidget* page = pages_->widget(0);
    pages_->removeWidget(page);
    page->deleteLater();
  }

  PropertyGroup& root = model_.GetRootGroup();

  // Split the root group's entries into loose leaf properties (a "General" tab)
  // and subgroups (one tab each), preserving order.
  std::vector<int> general_fields;
  std::vector<std::pair<QString, PropertyGroup*>> sections;
  for (int i = 0; i < root.GetCount(); ++i) {
    if (IsSection(root, i))
      sections.emplace_back(QString::fromStdU16String(root.GetName(i)),
                            root.GetSubgroup(i));
    else
      general_fields.push_back(i);
  }

  auto add_subtab = [&](const QString& name, int page_index) {
    auto* button = new QPushButton{name};
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(
        QStringLiteral(
            "QPushButton{background:transparent;border:none;color:%1;"
            "padding:4px 10px;border-radius:4px;}"
            "QPushButton:checked{background:%2;color:%3;font-weight:600;}")
            .arg(tokens.fg_subtle.name(), tokens.surface_muted.name(),
                 tokens.fg.name()));
    connect(button, &QPushButton::clicked, this,
            [this, page_index] { SelectSection(page_index); });
    subtab_layout_->addWidget(button);
    subtab_buttons_.push_back(button);
  };

  if (!general_fields.empty()) {
    const int page_index = pages_->count();
    pages_->addWidget(BuildSectionPage(root, general_fields));
    add_subtab(Tr("General"), page_index);
  }

  for (const auto& [name, group] : sections) {
    std::vector<int> field_indices;
    for (int j = 0; j < group->GetCount(); ++j) {
      if (!IsSection(*group, j))
        field_indices.push_back(j);
    }
    const int page_index = pages_->count();
    pages_->addWidget(BuildSectionPage(*group, field_indices));
    add_subtab(name, page_index);
  }

  // The device's address-map preview, after the property-group tabs.
  if (!address_map_.empty()) {
    const int page_index = pages_->count();
    pages_->addWidget(BuildAddressMapPage());
    add_subtab(Tr("Address map"), page_index);
  }

  if (pages_->count() > 0)
    SelectSection(0);
  UpdateDirtyUi();
}

void DeviceParameterForm::SetAddressMap(std::vector<AddressMapRow> rows) {
  address_map_ = std::move(rows);
  Rebuild();
}

QWidget* DeviceParameterForm::BuildAddressMapPage() {
  const scada::aui::ThemeTokens& tokens = FormTokens();

  auto* table = new QTableWidget;
  table->setObjectName(QStringLiteral("addressMapTable"));
  table->setEditTriggers(QAbstractItemView::NoEditTriggers);  // read-only.
  table->setSelectionMode(QAbstractItemView::NoSelection);
  table->setColumnCount(4);
  const QStringList headers{Tr("Signal"), Tr("Type"), Tr("IOA"), Tr("NodeId")};
  table->setHorizontalHeaderLabels(headers);
  table->verticalHeader()->setVisible(false);
  table->horizontalHeader()->setStretchLastSection(true);
  table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
  table->setStyleSheet(
      QStringLiteral("QTableWidget{background:%1;color:%2;border:none;"
                     "gridline-color:%3;}"
                     "QHeaderView::section{background:%4;color:%5;border:none;"
                     "border-bottom:1px solid %3;padding:4px 8px;}")
          .arg(tokens.bg.name(), tokens.fg.name(), tokens.border.name(),
               tokens.surface_muted.name(), tokens.fg_subtle.name()));

  table->setRowCount(static_cast<int>(address_map_.size()));
  for (int row = 0; row < static_cast<int>(address_map_.size()); ++row) {
    const AddressMapRow& data = address_map_[row];
    const QString cells[] = {QString::fromStdU16String(data.signal),
                             QString::fromStdU16String(data.type),
                             QString::fromStdU16String(data.ioa),
                             QString::fromStdU16String(data.node_id)};
    for (int col = 0; col < 4; ++col) {
      auto* item = new QTableWidgetItem{cells[col]};
      item->setFlags(Qt::ItemIsEnabled);
      table->setItem(row, col, item);
    }
  }

  return table;
}

void DeviceParameterForm::OnFieldEdited(const Field& field,
                                        const QString& text) {
  const ParameterStaging::Key key = FieldKey(field);
  // Stage only a real change; typing the live value back clears the edit so the
  // form does not read as dirty when nothing differs.
  if (text.toStdU16String() == field.group->GetValue(field.index))
    staging_.Remove(key);
  else
    staging_.Set(key, text.toStdU16String());
  UpdateDirtyUi();
}

void DeviceParameterForm::UpdateDirtyUi() {
  const bool is_dirty = staging_.dirty();
  revert_->setEnabled(is_dirty);
  apply_->setEnabled(is_dirty);
  dirty_dot_->setVisible(is_dirty);
}

void DeviceParameterForm::SelectSection(int index) {
  pages_->setCurrentIndex(index);
  for (size_t i = 0; i < subtab_buttons_.size(); ++i)
    subtab_buttons_[i]->setChecked(static_cast<int>(i) == index);
}

void DeviceParameterForm::Apply() {
  // Replay the staged edits onto the model's write path. SetValue may complete
  // asynchronously; the field editors already show the applied text, and the
  // model's change handler refreshes them when the write lands.
  for (const auto& [key, value] : staging_.edits()) {
    auto* group = const_cast<PropertyGroup*>(
        static_cast<const PropertyGroup*>(key.first));
    group->SetValue(key.second, value);
  }
  staging_.Clear();
  UpdateDirtyUi();
}

void DeviceParameterForm::Revert() {
  staging_.Clear();
  for (const Field& field : fields_) {
    SetEditorText(
        field, QString::fromStdU16String(field.group->GetValue(field.index)));
  }
  UpdateDirtyUi();
}

DeviceParameterForm* MakeDeviceParameterForm(scada::aui::PropertyModel& model,
                                             QString title) {
  if (scada::aui::GetSeverityTheme() == scada::aui::SeverityTheme::kLegacy)
    return nullptr;
  return new DeviceParameterForm(model, std::move(title));
}
