#include "events/qt/area_sidebar.h"

#include "aui/qt/message_loop_qt.h"
#include "aui/test/app_environment.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QListWidget>

#include <memory>
#include <optional>
#include <vector>

namespace {

// Signal-capable stand-in for the journal model; the sidebar only subscribes
// to change notifications.
class StubTableModel : public scada::aui::TableModel {
 public:
  virtual int GetRowCount() override { return 0; }
  virtual void GetCell(scada::aui::TableCell& cell) override {}

  void NotifyChanged() { NotifyModelChanged(); }
  void NotifyItemAdded(int first, int count) { NotifyItemsAdded(first, count); }
};

class EventAreaSidebarTest : public testing::Test {
 protected:
  QListWidget* MakeSidebar() {
    auto* sidebar = MakeEventAreaSidebar(EventAreaSidebarContext{
        .executor = executor_,
        .model = model_,
        .browse_areas = [this] { return BrowseFixedAreas(); },
        .counts =
            [this](std::span<const scada::NodeId> areas) {
              ++count_calls_;
              EventTableModel::AreaCounts counts = counts_;
              counts.per_area.resize(areas.size(), 0);
              return counts;
            },
        .on_area =
            [this](const std::optional<scada::NodeId>& area) {
              chosen_.push_back(area);
            }});
    auto* list = qobject_cast<QListWidget*>(sidebar);
    // Pump until the async browse fills the area rows.
    for (int i = 0; i < 200 && list->count() < 3; ++i)
      QApplication::processEvents(QEventLoop::WaitForMoreEvents);
    return list;
  }

  // Recounts are coalesced onto the executor (task 721), so a notification
  // reaches the rows on the next turn of the loop rather than inside the
  // signal. Everything asserting on the row text pumps first.
  void Pump() {
    for (int i = 0; i < 20; ++i)
      QApplication::processEvents();
  }

  Awaitable<std::vector<EventAreaEntry>> BrowseFixedAreas() {
    co_return std::vector<EventAreaEntry>{
        {scada::NodeId{100, 4}, u"КРУ"},
        {scada::NodeId{101, 4}, u"ЭСТРА-ПС"},
    };
  }

  AppEnvironment app_env_;
  AnyExecutor executor_ = MakeAnyExecutor(std::make_shared<MessageLoopQt>());
  StubTableModel model_;
  EventTableModel::AreaCounts counts_{.total = 0, .per_area = {}};
  int count_calls_ = 0;
  std::vector<std::optional<scada::NodeId>> chosen_;
};

// The sidebar lists "All areas" plus the browsed areas, and selecting a row
// publishes its scope (an area id, or nullopt for "All areas").
TEST_F(EventAreaSidebarTest, ListsAreasAndPublishesTheChosenScope) {
  QListWidget* list = MakeSidebar();
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(list->count(), 3);
  EXPECT_EQ(list->currentRow(), 0);

  list->setCurrentRow(1);
  ASSERT_EQ(chosen_.size(), 1u);
  ASSERT_TRUE(chosen_.back().has_value());
  EXPECT_EQ(*chosen_.back(), (scada::NodeId{100, 4}));

  list->setCurrentRow(0);
  ASSERT_EQ(chosen_.size(), 2u);
  EXPECT_FALSE(chosen_.back().has_value());

  delete list;
}

// A model notification re-reads the counts into the rows: a burdened area
// carries its count, the quiet ones stay plain, and "All areas" carries the
// total.
TEST_F(EventAreaSidebarTest, ModelChangeRefreshesTheCounts) {
  QListWidget* list = MakeSidebar();
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(list->count(), 3);

  counts_ = {.total = 3, .per_area = {2, 0}};
  model_.NotifyChanged();
  Pump();

  EXPECT_TRUE(list->item(0)->text().contains(QStringLiteral("3")));
  EXPECT_TRUE(list->item(1)->text().contains(QStringLiteral("2")));
  EXPECT_FALSE(list->item(2)->text().contains(QStringLiteral("·")));

  counts_ = {.total = 0, .per_area = {0, 0}};
  model_.NotifyChanged();
  Pump();
  EXPECT_FALSE(list->item(1)->text().contains(QStringLiteral("·")));

  delete list;
}

// Regression (backlog 721): a recount walks the journal's whole event set, and
// during a flood the journal notifies once per arriving event — so a batch of
// arrivals used to cost one full recount each. They now coalesce into one.
TEST_F(EventAreaSidebarTest, ABurstOfNotificationsCostsOneRecount) {
  QListWidget* list = MakeSidebar();
  ASSERT_NE(list, nullptr);
  Pump();

  count_calls_ = 0;
  counts_ = {.total = 40, .per_area = {40, 0}};
  for (int i = 0; i < 40; ++i)
    model_.NotifyItemAdded(i, 1);
  // Nothing has recounted yet: the burst is still in flight.
  EXPECT_EQ(count_calls_, 0);

  Pump();
  EXPECT_EQ(count_calls_, 1);
  EXPECT_TRUE(list->item(0)->text().contains(QStringLiteral("40")));

  // And the next burst is not swallowed by the first.
  counts_ = {.total = 41, .per_area = {41, 0}};
  model_.NotifyItemAdded(40, 1);
  Pump();
  EXPECT_EQ(count_calls_, 2);
  EXPECT_TRUE(list->item(0)->text().contains(QStringLiteral("41")));

  delete list;
}

}  // namespace
