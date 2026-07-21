#include "modules/transmission/qt/transmission_destination_rail.h"

#include "aui/models/grid_model.h"
#include "aui/qt/theme_qt.h"

#include <QListWidget>
#include <QPointer>

#include <boost/signals2/connection.hpp>

#include <memory>
#include <utility>

namespace {

// The display text of a row: the device name with its rule count appended
// ("Ретрансляция КП-02 MODBUS · 4"); a device with no rules stays plain.
QString DeviceRowText(const QString& name, int rule_count) {
  if (rule_count <= 0)
    return name;
  return name + QStringLiteral(" · ") + QString::number(rule_count);
}

}  // namespace

QWidget* MakeTransmissionDestinationRail(
    TransmissionDestinationRailContext context) {
  auto* rail = new QListWidget;
  rail->setObjectName(QStringLiteral("transmissionDestinationRail"));
  rail->setFixedWidth(190);

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

  auto refresh_current_count = [list = QPointer<QListWidget>{rail},
                                current_count = context.current_count, entries,
                                current] {
    if (!list || !current_count)
      return;
    for (size_t i = 0; i < entries->size(); ++i) {
      if ((*entries)[i].node_id != *current)
        continue;
      (*entries)[i].rule_count = current_count();
      if (auto* item = list->item(static_cast<int>(i))) {
        item->setText(
            DeviceRowText(QString::fromStdU16String((*entries)[i].name),
                          (*entries)[i].rule_count));
      }
      break;
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
             entries, current]() -> Awaitable<void> {
              auto devices = co_await browse();
              if (!list)
                co_return;
              *entries = std::move(devices);
              for (const TransmissionDeviceEntry& entry : *entries) {
                list->addItem(DeviceRowText(
                    QString::fromStdU16String(entry.name), entry.rule_count));
                if (entry.node_id == *current)
                  list->setCurrentRow(list->count() - 1);
              }
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
    QObject::connect(rail, &QObject::destroyed,
                     [connections]() mutable { connections->clear(); });
  }

  return rail;
}
