#include "transmission/qt/transmission_destination_rail.h"

#include "aui/models/grid_model.h"
#include "aui/qt/message_loop_qt.h"
#include "aui/test/app_environment.h"

#include <gtest/gtest.h>

#include <QApplication>
#include <QListWidget>

#include <memory>
#include <vector>

namespace {

// Signal-capable stand-in for the rules grid; the rail only subscribes to
// change notifications.
class StubGridModel : public scada::aui::GridModel {
 public:
  virtual void GetCell(scada::aui::GridCell& cell) override {}

  void NotifyChanged() { NotifyModelChanged(); }
  // What an address edit announces: rows repainted, none added or removed.
  void NotifyRowsRepainted() { NotifyRowsChanged(0, 1); }
};

class TransmissionDestinationRailTest : public testing::Test {
 protected:
  QListWidget* MakeRail() {
    auto* rail =
        MakeTransmissionDestinationRail(TransmissionDestinationRailContext{
            .executor = executor_,
            .browse = [this] { return BrowseFixedDevices(); },
            .current = scada::NodeId{702, 4},
            .current_count = [this] { return current_count_; },
            .current_conflicts = [this] { return current_conflicts_; },
            .model = &model_,
            .on_device =
                [this](const scada::NodeId& id) { chosen_.push_back(id); }});
    auto* list = qobject_cast<QListWidget*>(rail);
    // Pump until the async browse fills the device rows.
    for (int i = 0; i < 200 && list->count() < 2; ++i)
      QApplication::processEvents(QEventLoop::WaitForMoreEvents);
    return list;
  }

  Awaitable<std::vector<TransmissionDeviceEntry>> BrowseFixedDevices() {
    co_return std::vector<TransmissionDeviceEntry>{
        {scada::NodeId{702, 4}, u"Ретрансляция КП-02", 4},
        {scada::NodeId{703, 4}, u"Ретрансляция КП-05", 2},
    };
  }

  AppEnvironment app_env_;
  AnyExecutor executor_ = MakeAnyExecutor(std::make_shared<MessageLoopQt>());
  StubGridModel model_;
  int current_count_ = 4;
  int current_conflicts_ = 0;
  std::vector<scada::NodeId> chosen_;
};

// The rail lists the browsed devices with their rule counts, preselects the
// open device without firing the switch, and publishes a pick of the other
// device.
TEST_F(TransmissionDestinationRailTest, ListsDevicesAndPublishesThePick) {
  QListWidget* list = MakeRail();
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(list->count(), 2);
  EXPECT_EQ(list->currentRow(), 0);
  EXPECT_TRUE(list->item(0)->text().contains(QStringLiteral("4")));
  EXPECT_TRUE(list->item(1)->text().contains(QStringLiteral("2")));
  EXPECT_TRUE(chosen_.empty());

  list->setCurrentRow(1);
  ASSERT_EQ(chosen_.size(), 1u);
  EXPECT_EQ(chosen_.back(), (scada::NodeId{703, 4}));

  delete list;
}

// A grid-model notification re-reads the open device's rule count into its
// row, so an added/deleted rule updates the rail.
TEST_F(TransmissionDestinationRailTest, ModelChangeRefreshesTheOpenCount) {
  QListWidget* list = MakeRail();
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(list->count(), 2);

  current_count_ = 5;
  model_.NotifyChanged();
  EXPECT_TRUE(list->item(0)->text().contains(QStringLiteral("5")));
  // The other device's count is untouched.
  EXPECT_TRUE(list->item(1)->text().contains(QStringLiteral("2")));

  delete list;
}

// Backlog 1128: rules sharing a destination address corrupt what the
// dispatch centre receives, so the open device's rail row carries their count
// behind a warning mark -- a shape and a number -- and names it on hover. An
// address edit (rows repainted, none added) can make or
// clear a conflict, so it refreshes the row too.
TEST_F(TransmissionDestinationRailTest, ConflictsMarkTheOpenDevicesRow) {
  current_conflicts_ = 2;
  QListWidget* list = MakeRail();
  ASSERT_NE(list, nullptr);
  ASSERT_EQ(list->count(), 2);

  // A long device name elides in the middle, keeping the counts at the end.
  EXPECT_EQ(list->textElideMode(), Qt::ElideMiddle);
  // Read on fill, without waiting for the grid's next change.
  EXPECT_TRUE(list->item(0)->text().endsWith(QStringLiteral("\u26A0 2")))
      << list->item(0)->text().toStdString();
  EXPECT_FALSE(list->item(0)->toolTip().isEmpty());
  // The other device's conflicts are unknown, so it carries no mark.
  EXPECT_FALSE(list->item(1)->text().contains(QStringLiteral("\u26A0")));

  current_conflicts_ = 0;
  model_.NotifyRowsRepainted();
  EXPECT_FALSE(list->item(0)->text().contains(QStringLiteral("\u26A0")));
  EXPECT_TRUE(list->item(0)->text().contains(QStringLiteral("4")));
  EXPECT_TRUE(list->item(0)->toolTip().isEmpty());

  delete list;
}

}  // namespace
