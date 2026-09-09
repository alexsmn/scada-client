
#include "aui/models/mirror_table_model.h"

#include "aui/models/mirror_table_model.h"
#include "aui/test/recording_table_model_observer.h"
#include "base/u16format.h"

#include <gmock/gmock.h>

using namespace testing;

namespace scada::aui {

class TestTableModel : public TableModel {
 public:
  virtual int GetRowCount() override { return row_count; }

  virtual void GetCell(TableCell& cell) override {
    cell.text = u16format(L"{}:{}", cell.row, cell.column_id);
  }

  using TableModel::NotifyItemsAdded;
  using TableModel::NotifyItemsAdding;
  using TableModel::NotifyItemsChanged;
  using TableModel::NotifyItemsRemoved;
  using TableModel::NotifyItemsRemoving;

  int row_count = 0;
};

class MirrorTableModelTest : public Test {
 public:
  // Test
  virtual void SetUp() override;

 protected:
  TestTableModel source_model_;
  MirrorTableModel model_{source_model_};
  RecordingTableModelObserver observer_{model_};
};

void MirrorTableModelTest::SetUp() {
  model_.SetMirrored(true);
  observer_.ClearEvents();
}

TEST_F(MirrorTableModelTest, GetRowCount) {
  EXPECT_EQ(0, model_.GetRowCount());
  source_model_.row_count = 1;
  EXPECT_EQ(1, model_.GetRowCount());
  source_model_.row_count = 5;
  EXPECT_EQ(5, model_.GetRowCount());
}

TEST_F(MirrorTableModelTest, GetCell) {
  source_model_.row_count = 3;
  EXPECT_EQ(u"2:0", model_.GetCellText(0, 0));
  EXPECT_EQ(u"1:0", model_.GetCellText(1, 0));
  EXPECT_EQ(u"0:0", model_.GetCellText(2, 0));
  EXPECT_EQ(u"2:1", model_.GetCellText(0, 1));
  EXPECT_EQ(u"1:1", model_.GetCellText(1, 1));
  EXPECT_EQ(u"0:1", model_.GetCellText(2, 1));
}

TEST_F(MirrorTableModelTest, AddItemsToEmpty) {
  const int kAddCount = 3;
  source_model_.NotifyItemsAdding(0, kAddCount);
  EXPECT_THAT(observer_.items_adding, ElementsAre(Pair(0, kAddCount)));
  source_model_.row_count = kAddCount;
  source_model_.NotifyItemsAdded(0, kAddCount);
  EXPECT_THAT(observer_.items_added, ElementsAre(Pair(0, kAddCount)));
}

TEST_F(MirrorTableModelTest, AddItemsAtFront) {
  const int kInitCount = 7;
  const int kAddCount = 2;
  source_model_.row_count = kInitCount;
  source_model_.NotifyItemsAdding(0, kAddCount);
  EXPECT_THAT(observer_.items_adding, ElementsAre(Pair(kInitCount, kAddCount)));
  source_model_.row_count = kInitCount + kAddCount;
  source_model_.NotifyItemsAdded(0, kAddCount);
  EXPECT_THAT(observer_.items_added, ElementsAre(Pair(kInitCount, kAddCount)));
}

TEST_F(MirrorTableModelTest, AddItemsAtEnd) {
  const int kInitCount = 4;
  const int kAddCount = 6;
  source_model_.row_count = kInitCount;
  source_model_.NotifyItemsAdding(kInitCount, kAddCount);
  EXPECT_THAT(observer_.items_adding, ElementsAre(Pair(0, kAddCount)));
  source_model_.row_count = kInitCount + kAddCount;
  source_model_.NotifyItemsAdded(kInitCount, kAddCount);
  EXPECT_THAT(observer_.items_added, ElementsAre(Pair(0, kAddCount)));
}

// Source rows [0, 3) of 10 are mirrored rows [7, 10). These two cases used to
// assert the off-by-one they were meant to catch (`kInitCount - kRemoveCount -
// 1`), so the model reported row 6 for a block that starts at 7.
TEST_F(MirrorTableModelTest, RemoveItemsFromFront) {
  const int kInitCount = 10;
  const int kRemoveCount = 3;
  source_model_.row_count = kInitCount;
  source_model_.NotifyItemsRemoving(0, kRemoveCount);
  EXPECT_THAT(observer_.items_removing,
              ElementsAre(Pair(kInitCount - kRemoveCount, kRemoveCount)));
  source_model_.row_count = kInitCount - kRemoveCount;
  source_model_.NotifyItemsRemoved(0, kRemoveCount);
  EXPECT_THAT(observer_.items_removed,
              ElementsAre(Pair(kInitCount - kRemoveCount, kRemoveCount)));
}

// The tail of the source is the top of the mirror: source rows [2, 7) of 7
// are mirrored rows [0, 5). This is the shape TimedDataModel::UpdateRows
// always produces when the range shrinks, and it used to come out as row -1.
TEST_F(MirrorTableModelTest, RemoveItemsFromEnd) {
  const int kInitCount = 7;
  const int kRemoveCount = 5;
  source_model_.row_count = kInitCount;
  source_model_.NotifyItemsRemoving(kInitCount - kRemoveCount, kRemoveCount);
  EXPECT_THAT(observer_.items_removing, ElementsAre(Pair(0, kRemoveCount)));
  source_model_.row_count = kInitCount - kRemoveCount;
  source_model_.NotifyItemsRemoved(kInitCount - kRemoveCount, kRemoveCount);
  EXPECT_THAT(observer_.items_removed, ElementsAre(Pair(0, kRemoveCount)));
}

TEST_F(MirrorTableModelTest, ChangeSingleItemMapsToItsMirror) {
  source_model_.row_count = 10;
  source_model_.NotifyItemsChanged(9, 1);
  EXPECT_THAT(observer_.items_changed, ElementsAre(Pair(0, 1)));
}

// A batch changed at the source's tail is the top of the mirrored table. The
// single-row mapping (N - 1 - first) put it at [2, 5) instead, so the newest
// rows of a descending view never repainted.
TEST_F(MirrorTableModelTest, ChangeMultipleItemsMapsTheWholeBlock) {
  source_model_.row_count = 10;
  source_model_.NotifyItemsChanged(7, 3);
  EXPECT_THAT(observer_.items_changed, ElementsAre(Pair(0, 3)));
}

}  // namespace scada::aui
