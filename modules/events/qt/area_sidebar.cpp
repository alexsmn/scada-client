#include "modules/events/qt/area_sidebar.h"

#include "aui/qt/theme_qt.h"
#include "aui/translation.h"

#include <QListWidget>
#include <QPointer>
#include <QVBoxLayout>

#include <boost/signals2/connection.hpp>

#include <memory>
#include <utility>

namespace {

QString Tr(std::string_view text) {
  return QString::fromStdU16String(Translate(text));
}

// The display text of a row: the area name, with its unacknowledged count
// appended while non-zero ("КРУ · 2"), so a burdened area reads at a glance.
QString AreaRowText(const QString& name, int unacknowledged) {
  if (unacknowledged <= 0)
    return name;
  return name + QStringLiteral(" · ") + QString::number(unacknowledged);
}

}  // namespace

QWidget* MakeEventAreaSidebar(EventAreaSidebarContext context) {
  auto* sidebar = new QListWidget;
  sidebar->setObjectName(QStringLiteral("eventAreaSidebar"));
  sidebar->setFixedWidth(190);

  const scada::aui::ThemeTokens& tokens = scada::aui::ActiveThemeTokens();
  sidebar->setStyleSheet(
      QStringLiteral(
          "#eventAreaSidebar{background:%1;border:none;"
          "border-right:1px solid %2;color:%3;}"
          "#eventAreaSidebar::item{padding:6px 10px;}"
          "#eventAreaSidebar::item:selected{background:%4;color:%5;}")
          .arg(tokens.bg_elevated.name(), tokens.border.name(),
               tokens.fg_muted.name(), tokens.accent_soft.name(QColor::HexArgb),
               tokens.fg.name()));

  // Row 0 is the "All areas" scope; area rows follow in browse order, their
  // node ids held alongside.
  sidebar->addItem(Tr("All areas"));
  sidebar->setCurrentRow(0);
  auto area_ids = std::make_shared<std::vector<scada::NodeId>>();
  auto area_names = std::make_shared<std::vector<QString>>();

  auto refresh_counts = [list = QPointer<QListWidget>{sidebar},
                         counts = context.counts, area_ids, area_names,
                         all_areas = Tr("All areas")] {
    if (!list || !counts)
      return;
    const EventTableModel::AreaCounts area_counts = counts(*area_ids);
    list->item(0)->setText(AreaRowText(all_areas, area_counts.total));
    for (size_t i = 0; i < area_ids->size() && i < area_counts.per_area.size();
         ++i) {
      if (auto* item = list->item(static_cast<int>(i) + 1))
        item->setText(AreaRowText((*area_names)[i], area_counts.per_area[i]));
    }
  };

  QObject::connect(sidebar, &QListWidget::currentRowChanged, sidebar,
                   [area_ids, on_area = context.on_area](int row) {
                     if (!on_area)
                       return;
                     if (row <= 0)
                       on_area(std::nullopt);
                     else if (static_cast<size_t>(row - 1) < area_ids->size())
                       on_area((*area_ids)[row - 1]);
                   });

  // Fill the areas asynchronously, then bring the counts up.
  if (context.browse_areas) {
    CoSpawn(
        context.executor,
        [browse = context.browse_areas, list = QPointer<QListWidget>{sidebar},
         area_ids, area_names, refresh_counts]() -> Awaitable<void> {
          auto areas = co_await browse();
          if (!list)
            co_return;
          for (const EventAreaEntry& entry : areas) {
            area_ids->push_back(entry.node_id);
            area_names->push_back(QString::fromStdU16String(entry.name));
            list->addItem(area_names->back());
          }
          refresh_counts();
        });
  }

  // Keep the counts current on every journal notification, the connections
  // dying with the widget.
  auto connections =
      std::make_shared<std::vector<boost::signals2::scoped_connection>>();
  connections->push_back(context.model.SubscribeModelChanged(refresh_counts));
  auto on_range = [refresh_counts](int, int) { refresh_counts(); };
  connections->push_back(context.model.SubscribeItemsChanged(on_range));
  connections->push_back(context.model.SubscribeItemsAdded(on_range));
  connections->push_back(context.model.SubscribeItemsRemoved(on_range));
  QObject::connect(sidebar, &QObject::destroyed,
                   [connections]() mutable { connections->clear(); });

  refresh_counts();
  return sidebar;
}
