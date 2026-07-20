#include "modules/table/qt/table_toolbar.h"

#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QToolButton>

namespace {

// The design tokens for the active reshell theme. The bar is only built under
// a token theme (MakeTableToolbar gates on it), so the fallback is harmless.
const scada::aui::ThemeTokens& BarTokens() {
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

}  // namespace

TableToolbar::TableToolbar(TableToolbarContext context, QWidget* parent)
    : QWidget{parent}, context_{std::move(context)} {
  setObjectName(QStringLiteral("tableToolbar"));

  const scada::aui::ThemeTokens& tokens = BarTokens();
  setStyleSheet(
      QStringLiteral(
          "#tableToolbar{background:%1;border-bottom:1px solid %2;}"
          "#tableToolbar QToolButton{color:%3;background:transparent;"
          "border:1px solid transparent;border-radius:4px;padding:3px 8px;}"
          "#tableToolbar QToolButton:hover{background:%4;}"
          "#tableToolbar QToolButton:checked{background:%4;color:%5;}"
          "#tableToolbar QToolButton:disabled{color:%6;}"
          "#tableToolbar QLabel{color:%6;}")
          .arg(tokens.bg_elevated.name(), tokens.border.name(),
               tokens.fg_muted.name(), tokens.accent_soft.name(QColor::HexArgb),
               tokens.accent.name(), tokens.fg_subtle.name()));

  auto* layout = new QHBoxLayout{this};
  layout->setContentsMargins(10, 5, 10, 5);
  layout->setSpacing(6);

  // Add-signal is the view's own affordance (the trailing entry row), not a
  // shell command, so it is always present.
  auto* add = new QToolButton{this};
  add->setObjectName(QStringLiteral("tableToolbarAdd"));
  add->setText(QStringLiteral("+ ") + Tr("Add signal"));
  connect(add, &QToolButton::clicked, this, [this] {
    if (context_.on_add_signal)
      context_.on_add_signal();
  });
  layout->addWidget(add);

  layout->addWidget(AddCommandButton(ID_DELETE, Tr("Delete Row")));
  layout->addWidget(
      AddCommandButton(ID_MOVE_UP, QStringLiteral("↑ ") + Tr("Move Up")));
  layout->addWidget(
      AddCommandButton(ID_MOVE_DOWN, QStringLiteral("↓ ") + Tr("Move Down")));

  // Sort keys, checkable so the active key reads at a glance (same commands as
  // the context menu's Sort submenu).
  layout->addWidget(new QLabel{Tr("Sort"), this});
  layout->addWidget(AddCommandButton(ID_SORT_NAME, Tr("Name"),
                                     /*checkable=*/true));
  layout->addWidget(AddCommandButton(ID_SORT_CHANNEL, Tr("Channel"),
                                     /*checkable=*/true));

  layout->addStretch(1);

  // Selection/export commands, resolved through the shell surface.
  layout->addWidget(AddCommandButton(ID_OPEN_GRAPH, Tr("To graph")));
  layout->addWidget(AddCommandButton(ID_EXPORT_CSV, QStringLiteral("CSV")));
  layout->addWidget(AddCommandButton(ID_PRINT, Tr("Print")));

  Refresh();
}

TableToolbar::~TableToolbar() = default;

void TableToolbar::showEvent(QShowEvent* event) {
  QWidget::showEvent(event);
  Refresh();
  // Becoming the current tab shows the widget before the shell records it as
  // the active view, so the synchronous refresh can still read the previous
  // view's command surface — re-read once activation has settled.
  QMetaObject::invokeMethod(this, [this] { Refresh(); }, Qt::QueuedConnection);
}

QToolButton* TableToolbar::AddCommandButton(unsigned command_id,
                                            const QString& label,
                                            bool checkable) {
  auto* button = new QToolButton{this};
  button->setObjectName(QStringLiteral("tableToolbarCmd%1").arg(command_id));
  button->setText(label);
  button->setCheckable(checkable);
  connect(button, &QToolButton::clicked, this,
          [this, command_id] { ExecuteCommand(command_id); });
  command_buttons_.push_back({command_id, button, checkable});
  return button;
}

void TableToolbar::ExecuteCommand(unsigned command_id) {
  if (context_.resolve_command) {
    CommandHandler* handler = context_.resolve_command(command_id);
    if (handler && handler->IsCommandEnabled(command_id))
      handler->ExecuteCommand(command_id);
  }
  Refresh();
}

void TableToolbar::Refresh() {
  for (const CommandButton& entry : command_buttons_) {
    CommandHandler* handler = context_.resolve_command
                                  ? context_.resolve_command(entry.command_id)
                                  : nullptr;
    entry.button->setVisible(handler != nullptr);
    if (!handler)
      continue;
    entry.button->setEnabled(handler->IsCommandEnabled(entry.command_id));
    if (entry.checkable)
      entry.button->setChecked(handler->IsCommandChecked(entry.command_id));
  }
}

TableToolbar* MakeTableToolbar(TableToolbarContext context) {
  if (scada::aui::GetSeverityTheme() == scada::aui::SeverityTheme::kLegacy)
    return nullptr;
  return new TableToolbar{std::move(context)};
}
