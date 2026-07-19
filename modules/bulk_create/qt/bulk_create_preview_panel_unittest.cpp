#include "bulk_create/qt/bulk_create_preview_panel.h"

#include "aui/test/app_environment.h"

#include <gtest/gtest.h>

#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QTableWidget>

namespace {

class BulkCreatePreviewPanelTest : public ::testing::Test {
 protected:
  AppEnvironment app_env_;

  static BulkCreateParams SampleParams() {
    BulkCreateParams params;
    params.name_template = u"TS{n} current";
    params.node_id_template = u"ns=2;s=RTU.TS{n}.I";
    params.start_index = 1;
    params.count = 12;
    params.index_step = 1;
    params.ioa_start = 4001;
    params.ioa_step = 1;
    return params;
  }
};

TEST_F(BulkCreatePreviewPanelTest, DefaultPatternPopulatesPreviewGrid) {
  BulkCreatePreviewPanel panel;
  auto* grid = panel.findChild<QTableWidget*>(QStringLiteral("previewGrid"));
  ASSERT_NE(grid, nullptr);
  // The default constructor seeds a 12-row template.
  EXPECT_EQ(grid->rowCount(), 12);
  EXPECT_EQ(panel.rows().size(), 12u);
}

TEST_F(BulkCreatePreviewPanelTest, ConflictRowIsFlaggedAndCountedInSummary) {
  BulkCreatePreviewPanel panel;
  panel.SetExistingNodeIds({u"ns=2;s=RTU.TS8.I"});
  panel.SetParams(SampleParams());

  const std::vector<BulkCreatePreviewRow>& rows = panel.rows();
  ASSERT_EQ(rows.size(), 12u);
  EXPECT_TRUE(rows[7].conflict);  // TS8
  EXPECT_FALSE(rows[0].conflict);

  auto* grid = panel.findChild<QTableWidget*>(QStringLiteral("previewGrid"));
  ASSERT_NE(grid, nullptr);
  EXPECT_EQ(grid->item(7, 4)->text(), QStringLiteral("exists"));
  EXPECT_EQ(grid->item(0, 4)->text(), QStringLiteral("new"));

  auto* summary = panel.findChild<QLabel*>(QStringLiteral("previewSummary"));
  ASSERT_NE(summary, nullptr);
  EXPECT_TRUE(summary->text().contains(QStringLiteral("11 new")));
  EXPECT_TRUE(summary->text().contains(QStringLiteral("1 conflict")));
}

TEST_F(BulkCreatePreviewPanelTest, EditingCountSpinLiveUpdatesTheGrid) {
  BulkCreatePreviewPanel panel;
  panel.SetParams(SampleParams());

  // Drive the Count spin directly so the valueChanged -> Refresh wiring is what
  // updates the grid, not an explicit Refresh call.
  auto* count = panel.findChild<QSpinBox*>(QStringLiteral("countSpin"));
  ASSERT_NE(count, nullptr);
  count->setValue(5);

  auto* grid = panel.findChild<QTableWidget*>(QStringLiteral("previewGrid"));
  ASSERT_NE(grid, nullptr);
  EXPECT_EQ(grid->rowCount(), 5);
  EXPECT_EQ(panel.rows().size(), 5u);
}

TEST_F(BulkCreatePreviewPanelTest, EditingNameTemplateReexpands) {
  BulkCreatePreviewPanel panel;
  panel.SetParams(SampleParams());

  auto* name = panel.findChild<QLineEdit*>(QStringLiteral("nameTemplate"));
  ASSERT_NE(name, nullptr);
  name->setText(QStringLiteral("AI{nn}"));  // textChanged -> Refresh

  ASSERT_FALSE(panel.rows().empty());
  EXPECT_EQ(panel.rows()[0].name, u"AI01");
}

}  // namespace
