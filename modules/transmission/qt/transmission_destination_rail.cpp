#include "modules/transmission/qt/transmission_destination_rail.h"

#include "aui/models/grid_model.h"
#include "aui/models/grid_range.h"
#include "aui/qt/theme_qt.h"
#include "aui/translation.h"

#include <QListWidget>
#include <QPointer>

#include <boost/signals2/connection.hpp>

#include <memory>
#include <utility>

namespace {

// The display text of a row: the device name with its rule count appended
// ("Ретрансляция КП-02 MODBUS · 4"); a device with no rules stays plain.
// Rules sharing an address add a warning mark and their count
// ("… · 4 · ⚠ 2"): a shape and a number, which need no colour to read
// (docs/client/ux/principles.md §5).
QString DeviceRowText(const QString& name, int rule_count, int conflicts) {
  QString text = name;
  if (rule_count > 0)
    text += QStringLiteral(" · ") + QString::number(rule_count);
  if (conflicts > 0)
    text += QStringLiteral(" · \u26A0 ") + QString::number(conflicts);
  return text;
}

// Names the conflict count in words on hover; a clean row has no tooltip.
// No colour: the only row with a count is the open device's, which is the
// selected row, and the rail's `::item:selected` colour wins over an item's
// foreground -- a warning colour set here never reached the screen.
void ApplyConflictTooltip(QListWidgetItem& item, int conflicts) {
  item.setToolTip(
      conflicts <= 0
          ? QString{}
          : QString::fromStdU16String(Translate("Rules sharing an address")) +
                QStringLiteral(": ") + QString::number(conflicts));
}

}  // namespace

QWidget* MakeTransmissionDestinationRail(
    TransmissionDestinationRailContext context) {
  auto* rail = new QListWidget;
  rail->setObjectName(QStringLiteral("transmissionDestinationRail"));
  // Wider than the journal's area rail: device names ("Ретрансляция КП-02
  // MODBUS · 4") run longer than area names. A too-long name clips rather
  // than growing a scrollbar -- in the middle, so the counts at the end stay
  // readable: right-elision cut the open device's rule count and its
  // duplicate-address mark off entirely (backlog 1128).
  rail->setFixedWidth(240);
  rail->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  rail->setTextElideMode(Qt::ElideMiddle);

  const scada::aui::ThemeTokens& tokens = scada::aui::ActiveThemeTokens();
  rail->setStyleSheet(
      QStringLiteral(
          "#transmissionDestinationRail{background:%1;border:none;"
          "border-right:1px solid %2;color:%3;}"
          "#transmissionDestinationRail::item{padding:6px 10px;}"
          "#transmissionDestinationRail::item:selected{background:%4;"
          "color:%5;}")
          .arg(tokens.bg_elevated.name(), tokens.border.name(),
               tokens.fg_muted.name(), tokens.accent_soft.name(QColor::HexArgb),
               tokens.fg.name()));

  // Rows in browse order, their entries held alongside; `current` tracks the
  // device the grid shows so a programmatic preselect does not re-fire it.
  auto entries = std::make_shared<std::vector<TransmissionDeviceEntry>>();
  auto current = std::make_shared<scada::NodeId>(context.current);

  // Rewrites the open device's row from the live counts. Every other row
  // shows no conflicts: they are counted only for the device whose rules are
  // open, so leaving a device drops its mark rather than freezing it.
  auto refresh_current_count = [list = QPointer<QListWidget>{rail},
                                current_count = context.current_count,
                                current_conflicts = context.current_conflicts,
                                entries, current] {
    if (!list || !current_count)
      return;
    for (size_t i = 0; i < entries->size(); ++i) {
      auto* item = list->item(static_cast<int>(i));
      if (!item)
        continue;
      const bool open = (*entries)[i].node_id == *current;
      if (open)
        (*entries)[i].rule_count = current_count();
      const int conflicts = open && current_conflicts ? current_conflicts() : 0;
      item->setText(DeviceRowText(QString::fromStdU16String((*entries)[i].name),
                                  (*entries)[i].rule_count, conflicts));
      ApplyConflictTooltip(*item, conflicts);
    }
  };

  QObject::connect(rail, &QListWidget::currentRowChanged, rail,
                   [entries, current, on_device = context.on_device,
                    refresh_current_count](int row) {
                     if (row < 0 || static_cast<size_t>(row) >= entries->size())
                       return;
                     const scada::NodeId& id = (*entries)[row].node_id;
                     if (id == *current)
                       return;
                     *current = id;
                     if (on_device)
                       on_device(id);
                     refresh_current_count();
                   });

  // Fill the devices asynchronously and preselect the open one.
  if (context.browse) {
    CoSpawn(context.executor,
            [browse = context.browse, list = QPointer<QListWidget>{rail},
             entries, current, refresh_current_count]() -> Awaitable<void> {
              auto devices = co_await browse();
              if (!list)
                co_return;
              *entries = std::move(devices);
              for (const TransmissionDeviceEntry& entry : *entries) {
                list->addItem(DeviceRowText(
                    QString::fromStdU16String(entry.name), entry.rule_count,
                    /*conflicts=*/0));
                if (entry.node_id == *current)
                  list->setCurrentRow(list->count() - 1);
              }
              // The grid may have settled before the browse returned: read
              // the open device's live counts now rather than at its next
              // change.
              refresh_current_count();
            });
  }

  // Keep the open device's count current as rules are added/removed in the
  // grid, the connections dying with the widget.
  if (context.model) {
    auto connections =
        std::make_shared<std::vector<boost::signals2::scoped_connection>>();
    connections->push_back(context.model->SubscribeModelChanged(
        [refresh_current_count](scada::aui::GridModel&) {
          refresh_current_count();
        }));
    connections->push_back(context.model->SubscribeRowsAdded(
        [refresh_current_count](scada::aui::GridModel&, int, int) {
          refresh_current_count();
        }));
    connections->push_back(context.model->SubscribeRowsRemoved(
        [refresh_current_count](scada::aui::GridModel&, int, int) {
          refresh_current_count();
        }));
    // An address edit repaints rows without adding or removing any, and can
    // make or clear a conflict.
    connections->push_back(context.model->SubscribeRangeChanged(
        [refresh_current_count](scada::aui::GridModel&,
                                const scada::aui::GridRange&) {
          refresh_current_count();
        }));
    QObject::connect(rail, &QObject::destroyed,
                     [connections]() mutable { connections->clear(); });
  }

  return rail;
}
