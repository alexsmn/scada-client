#include "events/qt/alarm_footer.h"

#include "aui/test/app_environment.h"
#include "controller/command_handler.h"
#include "resources/common_resources.h"

#include <gtest/gtest.h>

#include <QLabel>
#include <QPushButton>

#include <memory>
#include <vector>

namespace {

// A signal-capable stand-in for the journal model; the footer only subscribes
// to its change notifications.
class StubTableModel : public scada::aui::TableModel {
 public:
  virtual int GetRowCount() override { return 0; }
  virtual void GetCell(scada::aui::TableCell& cell) override {}

  void NotifyChanged() { NotifyModelChanged(); }
};

class FakeCommandHandler : public CommandHandler {
 public:
  virtual bool IsCommandEnabled(unsigned command_id) const override {
    return enabled;
  }
  virtual void ExecuteCommand(unsigned command_id) override {
    executed.push_back(command_id);
  }

  bool enabled = false;
  std::vector<unsigned> executed;
};

class AlarmFooterTest : public testing::Test {
 protected:
  QLabel* Summary(QWidget& footer) {
    return footer.findChild<QLabel*>(QStringLiteral("alarmFooterSummary"));
  }
  QPushButton* AckAll(QWidget& footer) {
    return footer.findChild<QPushButton*>(QStringLiteral("alarmFooterAckAll"));
  }

  AppEnvironment app_env_;
  StubTableModel model_;
  EventTableModel::AlarmSummary state_;
  FakeCommandHandler handler_;

  std::unique_ptr<QWidget> footer_{MakeAlarmFooter(AlarmFooterContext{
      .model = model_,
      .summary = [this] { return state_; },
      .acknowledge_all = [this]() -> CommandHandler* { return &handler_; }})};
};

// A quiet journal reads calm and offers no acknowledge action.
TEST_F(AlarmFooterTest, CalmStateReadsNoBacklogAndDisablesAckAll) {
  QLabel* summary = Summary(*footer_);
  ASSERT_NE(summary, nullptr);
  EXPECT_TRUE(summary->text().contains(QStringLiteral("No unacknowledged")));

  QPushButton* acknowledge = AckAll(*footer_);
  ASSERT_NE(acknowledge, nullptr);
  EXPECT_FALSE(acknowledge->isEnabled());
}

// A model notification re-reads the backlog into the summary and the button's
// enablement — arrivals and acknowledgements keep the strip current.
TEST_F(AlarmFooterTest, ModelChangeRefreshesSummaryAndEnablement) {
  state_ = {.unacknowledged = 3, .max_severity = scada::kSeverityCritical};
  handler_.enabled = true;
  model_.NotifyChanged();

  QLabel* summary = Summary(*footer_);
  ASSERT_NE(summary, nullptr);
  EXPECT_TRUE(summary->text().contains(QStringLiteral("3")));
  EXPECT_TRUE(summary->text().contains(QStringLiteral("Critical")));

  QPushButton* acknowledge = AckAll(*footer_);
  ASSERT_NE(acknowledge, nullptr);
  EXPECT_TRUE(acknowledge->isEnabled());
}

// The button executes the journal's Acknowledge-All command.
TEST_F(AlarmFooterTest, AckAllExecutesTheCommand) {
  handler_.enabled = true;
  model_.NotifyChanged();

  QPushButton* acknowledge = AckAll(*footer_);
  ASSERT_NE(acknowledge, nullptr);
  acknowledge->click();
  EXPECT_EQ(handler_.executed, std::vector<unsigned>{ID_ACKNOWLEDGE_ALL});
}

}  // namespace
