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

}  // namespace
