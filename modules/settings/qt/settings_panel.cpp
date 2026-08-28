#include "settings/qt/settings_panel.h"

#include "aui/models/menu_model.h"
#include "aui/translation.h"

#include <QCheckBox>
#include <QComboBox>
#include <QEvent>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QScrollArea>
#include <QSignalBlocker>
#include <QSplitter>
#include <QStyle>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>

namespace {

using scada::aui::MenuModel;

QString Ui(std::u16string_view text) {
  return QString::fromStdU16String(std::u16string{text});
}

// The child of a choice submenu that is currently selected, or -1 when none is
// — which happens for Colour scheme before the operator has picked one.
int CheckedChild(const MenuModel& submenu) {
  for (int i = 0; i < submenu.GetItemCount(); ++i) {
    if (submenu.GetTypeAt(i) == MenuModel::TYPE_SEPARATOR)
      continue;
    if (submenu.IsItemCheckedAt(i))
      return i;
  }
  return -1;
}

// A label drawn in the palette's disabled text colour — the platform's own
// "this is secondary" colour, rather than a grey this file picked. Used for the
// scope chips, the row descriptions and the counts.
QLabel* MakeMutedLabel(QWidget* parent, const QString& text) {
  auto* label = new QLabel{text, parent};
  QPalette palette = label->palette();
  palette.setColor(QPalette::WindowText,
                   palette.color(QPalette::Disabled, QPalette::WindowText));
  label->setPalette(palette);
  return label;
}

// The chip naming where the row's value is stored. A framed label rather than a
// drawn pill: the frame is the platform style's, so it reads as the host's
// chrome instead of as the web client's.
QLabel* MakeScopeChip(QWidget* parent, SettingScope scope) {
  QLabel* chip = MakeMutedLabel(parent, Ui(SettingScopeLabel(scope)));
  chip->setFrameShape(QFrame::StyledPanel);
  chip->setAlignment(Qt::AlignCenter);
  chip->setMargin(2);
  chip->setToolTip(
      Ui(Translate("Where this value is stored, and so what it follows.")));
  return chip;
}

}  // namespace

SettingsPanel::SettingsPanel(QWidget* parent, MenuModel& settings_menu)
    : QWidget{parent}, settings_menu_{settings_menu} {
  // Opaque and framed: the panel covers the workbench rather than floating
  // translucently over it, and without an explicit background a child widget
  // paints nothing and the shell shows through.
  setAutoFillBackground(true);
  setBackgroundRole(QPalette::Window);
  setFocusPolicy(Qt::StrongFocus);

  auto* frame = new QFrame{this};
  frame->setFrameShape(QFrame::StyledPanel);
  auto* outer = new QVBoxLayout{this};
  outer->setContentsMargins(0, 0, 0, 0);
  outer->addWidget(frame);

  auto* layout = new QVBoxLayout{frame};

  // The panel's own header. It exists because the panel covers the window's
  // chrome, including the tab strip that would otherwise name what is on
  // screen: with the workbench hidden, nothing else says where the operator is
  // or how to get back.
  auto* header = new QHBoxLayout;
  auto* heading = new QLabel{Ui(Translate("Settings")), frame};
  QFont heading_font = heading->font();
  heading_font.setBold(true);
  heading->setFont(heading_font);
  header->addWidget(heading);
  header->addStretch(1);

  auto* close_button = new QToolButton{frame};
  close_button->setObjectName(QStringLiteral("settings-close"));
  close_button->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
  // The label is the accessible name and the tooltip, not painted text — the
  // glyph is the platform's own close button.
  close_button->setToolTip(Ui(Translate("Close Settings")));
  close_button->setAccessibleName(Ui(Translate("Close Settings")));
  connect(close_button, &QToolButton::clicked, this, &QWidget::hide);
  header->addWidget(close_button);
  layout->addLayout(header);

  search_field_ = new QLineEdit{frame};
  search_field_->setObjectName(QStringLiteral("settings-search"));
  search_field_->setPlaceholderText(Ui(Translate("Search settings")));
  search_field_->setClearButtonEnabled(true);
  layout->addWidget(search_field_);

  auto* scope_row = new QHBoxLayout;
  scope_tabs_ = new QTabBar{frame};
  scope_tabs_->setObjectName(QStringLiteral("settings-scopes"));
  scope_tabs_->setDrawBase(false);
  scope_tabs_->setExpanding(false);
  scope_row->addWidget(scope_tabs_);
  scope_row->addStretch(1);
  count_label_ = MakeMutedLabel(frame, QString{});
  scope_row->addWidget(count_label_);
  layout->addLayout(scope_row);

  // The table of contents takes the side of the window the Explorer had, which
  // is what the shared screen specifies and what the panel is wide enough for
  // because it covers the Explorer rather than sitting beside it. A splitter
  // rather than a fixed width because the category names are translated and
  // Russian is longer.
  auto* splitter = new QSplitter{Qt::Horizontal, frame};

  category_list_ = new QListWidget{splitter};
  category_list_->setObjectName(QStringLiteral("settings-categories"));
  splitter->addWidget(category_list_);

  auto* scroll = new QScrollArea{splitter};
  scroll->setWidgetResizable(true);
  rows_host_ = new QWidget{scroll};
  rows_layout_ = new QVBoxLayout{rows_host_};
  rows_layout_->setAlignment(Qt::AlignTop);
  scroll->setWidget(rows_host_);
  splitter->addWidget(scroll);

  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);
  layout->addWidget(splitter, 1);

  connect(search_field_, &QLineEdit::textChanged, this,
          [this] { RebuildRows(); });
  connect(scope_tabs_, &QTabBar::currentChanged, this,
          [this] { RebuildRows(); });
  // Selecting a category scrolls to its heading rather than filtering to it:
  // the list is one scrolling surface, which is what lets a search cross
  // categories.
  connect(category_list_, &QListWidget::currentRowChanged, this,
          [this, scroll](int row) {
            if (row < 0)
              return;
            const QVariant anchor =
                category_list_->item(row)->data(Qt::UserRole);
            if (auto* widget = anchor.value<QWidget*>())
              scroll->ensureWidgetVisible(widget);
          });

  if (parent)
    parent->installEventFilter(this);

  ReloadCatalog();
  hide();
}

SettingsPanel::~SettingsPanel() = default;

void SettingsPanel::Open(int bottom_inset) {
  bottom_inset_ = bottom_inset;
  // Re-read rather than trust what was built last time: a module can have
  // registered a contribution, and the session's rights can have changed, since
  // the panel was last on screen.
  ReloadCatalog();
  FitToParent();
  show();
  raise();
  // Focus the search box, not the first control: an operator opening Settings
  // is looking for a setting, and typing is how they look.
  search_field_->setFocus(Qt::OtherFocusReason);
}

void SettingsPanel::SetBottomInset(int bottom_inset) {
  bottom_inset_ = bottom_inset;
  FitToParent();
}

void SettingsPanel::FitToParent() {
  QWidget* host = parentWidget();
  if (!host)
    return;
  // Everything above the reserved strip. The strip is the status bar, which
  // stays visible because it reports the session, the connection and the
  // server — none of which stops being true while preferences are open. Its
  // height is whatever the shell last stated: `Status Bar` is a row on this
  // surface, so the strip can come and go while the panel is covering the
  // window (see `SetBottomInset`).
  const int height = std::max(0, host->height() - bottom_inset_);
  setGeometry(0, 0, host->width(), height);
}

bool SettingsPanel::eventFilter(QObject* watched, QEvent* event) {
  // Unconditionally, rather than only while showing: a panel resized while
  // hidden costs a geometry assignment, and one that is not is a panel that
  // appears at the previous window's size for one frame the next time it opens.
  if (watched == parentWidget() && event->type() == QEvent::Resize)
    FitToParent();
  return QWidget::eventFilter(watched, event);
}

void SettingsPanel::keyPressEvent(QKeyEvent* event) {
  if (event->key() == Qt::Key_Escape) {
    hide();
    event->accept();
    return;
  }
  QWidget::keyPressEvent(event);
}

void SettingsPanel::ReloadCatalog() {
  if (reloading_)
    return;
  reloading_ = true;

  // Dynamic models populate here; without it Language and the module
  // contributions would come back empty.
  settings_menu_.MenuWillShow();
  catalog_ = BuildSettingsCatalog(settings_menu_);

  RebuildScopeTabs();
  RebuildRows();

  reloading_ = false;
}

void SettingsPanel::RebuildScopeTabs() {
  const std::optional<SettingScope> selected = SelectedScope();
  scopes_ = VisibleSettingScopes(catalog_);

  const QSignalBlocker blocker{scope_tabs_};
  while (scope_tabs_->count() > 0)
    scope_tabs_->removeTab(0);

  scope_tabs_->addTab(Ui(Translate("All")));
  int restore = 0;
  for (std::size_t i = 0; i < scopes_.size(); ++i) {
    scope_tabs_->addTab(Ui(SettingScopeLabel(scopes_[i])));
    if (selected && *selected == scopes_[i])
      restore = static_cast<int>(i) + 1;
  }
  scope_tabs_->setCurrentIndex(restore);
}

std::optional<SettingScope> SettingsPanel::SelectedScope() const {
  const int index = scope_tabs_ ? scope_tabs_->currentIndex() : 0;
  if (index <= 0 || static_cast<std::size_t>(index) > scopes_.size())
    return std::nullopt;
  return scopes_[static_cast<std::size_t>(index) - 1];
}

void SettingsPanel::RebuildRows() {
  const std::u16string query = search_field_->text().toStdU16String();
  visible_rows_ = FilterSettingRows(catalog_, query, SelectedScope());

  // Everything under the host is rebuilt rather than reconciled: the list is
  // fourteen rows at most, and a reconciler would be the only thing in this
  // file that could disagree with the catalogue.
  qDeleteAll(rows_host_->findChildren<QWidget*>(Qt::FindDirectChildrenOnly));

  category_list_->clear();

  for (const SettingCategoryGroup& group : GroupSettingRows(visible_rows_)) {
    auto* heading = new QLabel{Ui(group.label), rows_host_};
    QFont font = heading->font();
    font.setBold(true);
    heading->setFont(font);
    rows_layout_->addWidget(heading);

    auto* rule = new QFrame{rows_host_};
    rule->setFrameShape(QFrame::HLine);
    rule->setFrameShadow(QFrame::Sunken);
    rows_layout_->addWidget(rule);

    for (const SettingRow& row : group.rows)
      rows_layout_->addWidget(CreateRow(row));

    auto* entry =
        new QListWidgetItem{QStringLiteral("%1  (%2)")
                                .arg(Ui(group.label))
                                .arg(static_cast<int>(group.rows.size())),
                            category_list_};
    entry->setData(Qt::UserRole,
                   QVariant::fromValue(static_cast<QWidget*>(heading)));
  }

  const SettingRowCounts counts = CountSettingRows(visible_rows_);
  // Two numbers rather than one, because an action is not a setting: a single
  // total would report a button as a preference the operator holds.
  count_label_->setText(Ui(Translate("Settings: %1 · Actions: %2"))
                            .arg(counts.settings)
                            .arg(counts.actions));
}

QWidget* SettingsPanel::CreateRow(const SettingRow& row) {
  auto* widget = new QWidget{rows_host_};
  widget->setObjectName(
      QString::fromUtf8(row.id.data(), static_cast<qsizetype>(row.id.size())));
  auto* layout = new QVBoxLayout{widget};

  // A toggle's title is its checkbox's label, so drawing it above as well would
  // say the same thing twice; the chip moves onto the checkbox's own line
  // instead. A choice or an action has a control that cannot carry the title,
  // so those keep a heading of their own. The row's four parts — title, chip,
  // description, control — are the same either way, which is what the shared
  // screen specifies; where the title sits is this realm's own answer.
  auto* title_row = new QHBoxLayout;
  if (row.control == SettingControl::kToggle) {
    title_row->addWidget(CreateToggleControl(row, widget));
  } else {
    auto* title = new QLabel{Ui(row.title), widget};
    QFont font = title->font();
    font.setBold(true);
    title->setFont(font);
    title_row->addWidget(title);
  }
  title_row->addStretch(1);
  title_row->addWidget(MakeScopeChip(widget, row.scope));
  layout->addLayout(title_row);

  QLabel* description = MakeMutedLabel(widget, Ui(row.description));
  description->setWordWrap(true);
  layout->addWidget(description);

  switch (row.control) {
    case SettingControl::kChoice:
      layout->addWidget(CreateChoiceControl(row, widget));
      break;
    case SettingControl::kToggle:
      break;
    case SettingControl::kAction:
      layout->addWidget(CreateActionControl(row, widget));
      break;
  }

  return widget;
}

QWidget* SettingsPanel::CreateChoiceControl(const SettingRow& row,
                                            QWidget* parent) {
  auto* combo = new QComboBox{parent};
  MenuModel* submenu = row.model->GetSubmenuModelAt(row.index);
  if (!submenu)
    return combo;
  submenu->MenuWillShow();

  // The model index behind each entry, so a separator inside the submenu
  // cannot shift what a selection activates.
  for (int child = 0; child < submenu->GetItemCount(); ++child) {
    if (submenu->GetTypeAt(child) == MenuModel::TYPE_SEPARATOR)
      continue;
    if (!submenu->IsVisibleAt(child))
      continue;
    combo->addItem(Ui(submenu->GetLabelAt(child)), child);
  }

  if (const int checked = CheckedChild(*submenu); checked >= 0)
    combo->setCurrentIndex(combo->findData(checked));

  connect(combo, &QComboBox::activated, this,
          [this, submenu, combo](int entry) {
            const QVariant child = combo->itemData(entry);
            if (!child.isValid())
              return;
            submenu->ActivatedAt(child.toInt());
            // Colour scheme and Style change the application's look under this
            // widget, and Language re-translates every string on it, so the
            // panel is rebuilt rather than left showing what it was built from.
            //
            // **That rebuild deletes this combo — the sender — from inside its
            // own emission**, three calls down in `RebuildRows`, so nothing
            // here shows it. It is safe: Qt reference-counts a sender's
            // connection list across `activate`, which is why deleting the
            // sender in a slot is supported rather than merely tolerated. What
            // is not safe is touching `combo` after this line, so nothing does
            // — `child` is read before the rebuild for that reason.
            ReloadCatalog();
            // After the rebuild, not before: the shell re-measures against the
            // strings and metrics the choice just installed, not the ones it
            // replaced.
            emit SettingApplied();
          });

  return combo;
}

QCheckBox* SettingsPanel::CreateToggleControl(const SettingRow& row,
                                              QWidget* parent) {
  auto* box = new QCheckBox{Ui(row.title), parent};
  box->setChecked(row.model->IsItemCheckedAt(row.index));
  box->setEnabled(row.model->IsEnabledAt(row.index));
  // A command can be disabled by a rule of its own; saying why beats a greyed
  // row with no explanation.
  if (!row.model->IsEnabledAt(row.index)) {
    if (const std::u16string reason = row.model->GetDisabledReasonAt(row.index);
        !reason.empty()) {
      box->setToolTip(Ui(reason));
    }
  }

  // The command toggles, so it is activated rather than set — the same call the
  // menu item made. Re-reading the model afterwards keeps the box honest if the
  // command refused or the profile clamped the value.
  MenuModel* model = row.model;
  const int index = row.index;
  connect(box, &QCheckBox::clicked, this, [this, model, index, box] {
    model->ActivatedAt(index);
    box->setChecked(model->IsItemCheckedAt(index));
    emit SettingApplied();
  });

  return box;
}

QWidget* SettingsPanel::CreateActionControl(const SettingRow& row,
                                            QWidget* parent) {
  auto* button = new QPushButton{Ui(row.title), parent};
  button->setEnabled(row.model->IsEnabledAt(row.index));

  MenuModel* model = row.model;
  const int index = row.index;
  connect(button, &QPushButton::clicked, this, [this, model, index] {
    model->ActivatedAt(index);
    emit SettingApplied();
  });

  return button;
}
