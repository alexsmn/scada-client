#include "modules/events/qt/alarm_footer.h"

#include "aui/qt/theme_qt.h"
#include "aui/translation.h"
#include "controller/command_handler.h"
#include "modules/events/event_severity.h"
#include "resources/common_resources.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>

#include <memory>
#include <vector>

namespace {

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

}  // namespace

QWidget* MakeAlarmFooter(AlarmFooterContext context) {
  auto* footer = new QWidget;
  footer->setObjectName(QStringLiteral("alarmFooter"));

  const scada::aui::ThemeTokens& tokens = scada::aui::ActiveThemeTokens();
  footer->setStyleSheet(
      QStringLiteral("#alarmFooter{background:%1;border-top:1px solid %2;}"
                     "#alarmFooter QLabel{color:%3;}")
          .arg(tokens.bg_elevated.name(), tokens.border.name(),
               tokens.fg_muted.name()));

  auto* layout = new QHBoxLayout{footer};
  layout->setContentsMargins(10, 4, 10, 4);
  layout->setSpacing(12);

  auto* summary_label = new QLabel{footer};
  summary_label->setObjectName(QStringLiteral("alarmFooterSummary"));
  layout->addWidget(summary_label);
  layout->addStretch(1);

  auto* acknowledge = new QPushButton{Tr("Acknowledge All"), footer};
  acknowledge->setObjectName(QStringLiteral("alarmFooterAckAll"));
  acknowledge->setStyleSheet(
      QStringLiteral("QPushButton{background:%1;color:%2;border:none;"
                     "border-radius:4px;padding:3px 10px;font-weight:600;}"
                     "QPushButton:disabled{background:%3;color:%4;}")
          .arg(tokens.accent.name(), tokens.accent_fg.name(),
               tokens.surface_muted.name(), tokens.fg_subtle.name()));
  layout->addWidget(acknowledge);

  auto refresh = [summary = std::move(context.summary),
                  acknowledge_all = context.acknowledge_all, summary_label,
                  acknowledge, fg = tokens.fg.name(),
                  fg_muted = tokens.fg_muted.name()] {
    const EventTableModel::AlarmSummary state =
        summary ? summary() : EventTableModel::AlarmSummary{};
    summary_label->setText(QString::fromStdU16String(
        events::AlarmSummaryLabel(state.unacknowledged, state.max_severity)));
    // A live backlog reads emphasized; the calm zero state stays muted. The
    // text itself carries the state, the weight only reinforces it.
    summary_label->setStyleSheet(
        state.unacknowledged > 0
            ? QStringLiteral("color:%1;font-weight:600;").arg(fg)
            : QStringLiteral("color:%1;").arg(fg_muted));

    CommandHandler* handler = acknowledge_all ? acknowledge_all() : nullptr;
    acknowledge->setEnabled(handler &&
                            handler->IsCommandEnabled(ID_ACKNOWLEDGE_ALL));
  };

  QObject::connect(
      acknowledge, &QPushButton::clicked, footer,
      [acknowledge_all = context.acknowledge_all, refresh] {
        CommandHandler* handler = acknowledge_all ? acknowledge_all() : nullptr;
        if (handler && handler->IsCommandEnabled(ID_ACKNOWLEDGE_ALL)) {
          handler->ExecuteCommand(ID_ACKNOWLEDGE_ALL);
        }
        refresh();
      });

  // Track every model notification: arrivals, acknowledgements, refilters.
  // The connections must die with the widget, so the holder is tied to its
  // destruction.
  auto connections =
      std::make_shared<std::vector<boost::signals2::scoped_connection>>();
  connections->push_back(context.model.SubscribeModelChanged(refresh));
  auto on_range = [refresh](int, int) { refresh(); };
  connections->push_back(context.model.SubscribeItemsChanged(on_range));
  connections->push_back(context.model.SubscribeItemsAdded(on_range));
  connections->push_back(context.model.SubscribeItemsRemoved(on_range));
  QObject::connect(footer, &QObject::destroyed,
                   [connections]() mutable { connections->clear(); });

  refresh();
  return footer;
}
