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

// The grid's third status, and the proof that the panel's own spin boxes can
// reach it: ioa_start caps at 1e6, ioa_step at 1e5 and count at 1e5, so the
// last row of a large run runs past what the Address property can carry. It
// used to wrap to a negative number through signed overflow (backlog 719).
TEST_F(BulkCreatePreviewPanelTest, OutOfRangeIoaIsFlaggedAndCounted) {
  BulkCreatePreviewPanel panel;
  // The address half is the transmission branch's; a data item has no IOA to
  // put out of range.
  panel.SetSubject(BulkCreateSubject::kTransmissionItem);
  BulkCreateParams params = SampleParams();
  params.subject = BulkCreateSubject::kTransmissionItem;
  // 1e6 + 21464 * 1e5 = 2 147 400 000, the last address that fits; one step
  // further is 2 147 500 000, past INT32_MAX.
  params.ioa_start = 1000000;
  params.ioa_step = 100000;
  params.count = 21466;
  panel.SetParams(params);

  const std::vector<BulkCreatePreviewRow>& rows = panel.rows();
  ASSERT_EQ(rows.size(), 21466u);
  EXPECT_FALSE(rows[21464].ioa_out_of_range);
  EXPECT_EQ(rows[21464].ioa, 2147400000);
  EXPECT_TRUE(rows[21465].ioa_out_of_range);

  auto* grid = panel.findChild<QTableWidget*>(QStringLiteral("previewGrid"));
  ASSERT_NE(grid, nullptr);
  EXPECT_EQ(grid->item(21465, 4)->text(),
            QStringLiteral("address out of range"));

  auto* summary = panel.findChild<QLabel*>(QStringLiteral("previewSummary"));
  ASSERT_NE(summary, nullptr);
  EXPECT_TRUE(summary->text().contains(QStringLiteral("out of range")));
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

// One panel, two subjects: the address controls and the IOA column belong to
// the transmission branch alone, and a data item must not be offered an
// address it cannot carry.
TEST_F(BulkCreatePreviewPanelTest, TheIoaControlsAndColumnFollowTheSubject) {
  BulkCreatePreviewPanel panel;
  auto* grid = panel.findChild<QTableWidget*>(QStringLiteral("previewGrid"));
  ASSERT_NE(grid, nullptr);

  panel.SetSubject(BulkCreateSubject::kTransmissionItem);
  EXPECT_FALSE(grid->isColumnHidden(3));

  panel.SetSubject(BulkCreateSubject::kDataItem);
  EXPECT_TRUE(grid->isColumnHidden(3));
}

// Switching away and back must not silently reset the operator's addressing.
// The controls keep their values while hidden; ExpandBulkCreate ignores them
// for the other subject, so nothing stale can reach a data item either.
TEST_F(BulkCreatePreviewPanelTest, SwitchingSubjectKeepsTheAddressingValues) {
  BulkCreatePreviewPanel panel;
  panel.SetSubject(BulkCreateSubject::kTransmissionItem);
  BulkCreateParams params = SampleParams();
  params.subject = BulkCreateSubject::kTransmissionItem;
  params.ioa_start = 7000;
  params.ioa_step = 3;
  params.count = 2;
  panel.SetParams(params);
  ASSERT_EQ(panel.rows()[1].ioa, 7003);

  panel.SetSubject(BulkCreateSubject::kDataItem);
  EXPECT_TRUE(panel.rows()[1].ioa_absent);

  panel.SetSubject(BulkCreateSubject::kTransmissionItem);
  EXPECT_EQ(panel.rows()[1].ioa, 7003);
}

}  // namespace
