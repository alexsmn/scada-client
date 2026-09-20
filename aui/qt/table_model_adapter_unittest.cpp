#include "aui/qt/table_model_adapter.h"

#include "aui/models/table_model.h"
#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"

#include <QAbstractItemModelTester>
#include <QFont>
#include <QSignalSpy>
#include <QVariant>
#include <gtest/gtest.h>

namespace scada::aui {
namespace {

// A minimal in-memory model: one row of fixed text, enough to drive the
// adapter's per-column roles.
class StubTableModel : public TableModel {
 public:
  virtual int GetRowCount() override { return row_count; }
  virtual void GetCell(TableCell& cell) override {
    ++get_cell_calls;
    cell.text = u"42";
  }

  void AddItems(int first, int count) {
    ScopedItemsAdding adding{*this, first, count};
    row_count += count;
  }

  void ChangeWholesale() { NotifyModelChanged(); }

  int row_count = 1;
  int get_cell_calls = 0;
};

enum ColumnId { kTitleColumn, kValueColumn, kTimeColumn };

std::vector<TableColumn> MakeColumns() {
  return {
      {kTitleColumn, u"Title", 100, TableColumn::LEFT},
      {kValueColumn, u"Value", 100, TableColumn::RIGHT,
       TableColumn::DataType::General, /*monospace=*/true},
      {kTimeColumn, u"Time", 100, TableColumn::LEFT,
       TableColumn::DataType::DateTime},
  };
}

class TableModelAdapterTest : public testing::Test {
 protected:
  void TearDown() override { SetSeverityTheme(SeverityTheme::kDark); }

  QVariant FontFor(ColumnId column) {
    return adapter_.data(adapter_.index(0, column), Qt::FontRole);
  }

  int AlignmentFor(ColumnId column) {
    return adapter_.data(adapter_.index(0, column), Qt::TextAlignmentRole)
        .toInt();
  }

  AppEnvironment app_env_;
  std::shared_ptr<StubTableModel> model_ = std::make_shared<StubTableModel>();
  TableModelAdapter adapter_{model_, MakeColumns()};
};

// Under a token theme, value (monospace-flagged) and timestamp (Time)
// columns render in the fixed-pitch monospace value font so digits stay
// tabular as they update; other columns keep the default font.
TEST_F(TableModelAdapterTest, ValueAndTimestampColumnsRenderMonospace) {
  SetSeverityTheme(SeverityTheme::kDark);

  EXPECT_FALSE(FontFor(kTitleColumn).isValid());

  const QVariant value_font = FontFor(kValueColumn);
  ASSERT_TRUE(value_font.isValid());
  EXPECT_TRUE(value_font.value<QFont>().fixedPitch());

  const QVariant time_font = FontFor(kTimeColumn);
  ASSERT_TRUE(time_font.isValid());
  EXPECT_TRUE(time_font.value<QFont>().fixedPitch());
}

// A column's alignment carries its vertical half too. Regression: the adapter
// returned the horizontal flag alone, leaving the vertical bits zero — which
// Qt reads as AlignTop, so every table row sat a pixel higher than the same
// row in a grid.
TEST_F(TableModelAdapterTest, ColumnAlignmentIsVerticallyCentred) {
  EXPECT_EQ(AlignmentFor(kTitleColumn),
            static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter));
  EXPECT_EQ(AlignmentFor(kValueColumn),
            static_cast<int>(Qt::AlignRight | Qt::AlignVCenter));
}

// The per-column roles (font, alignment, tooltip) are answered before the
// cell is fetched, and so must the roles the adapter never answers at all —
// CheckStateRole used to cost a GetCell and then fall to the default.
TEST_F(TableModelAdapterTest, RolesThatDoNotReadTheCellDoNotFetchIt) {
  const QModelIndex index = adapter_.index(0, kValueColumn);
  adapter_.data(index, Qt::CheckStateRole);
  adapter_.data(index, Qt::SizeHintRole);
  adapter_.data(index, Qt::FontRole);
  EXPECT_EQ(model_->get_cell_calls, 0);

  adapter_.data(index, Qt::DisplayRole);
  EXPECT_EQ(model_->get_cell_calls, 1);
}

}  // namespace
// --- Structural change notification (backlog 712b) --------------------------

// `TableModel::ModelChanged` is documented as "changed wholesale", and the
// adapter answered it with a bare `layoutChanged()`. Behind `TableProxyModel`
// (a QSortFilterProxyModel) that is memory-unsafe rather than merely wrong:
// `_q_sourceLayoutChanged` deletes every mapping and remaps only the
// persistent indexes the never-emitted `layoutAboutToBeChanged()` would have
// saved, leaving the view's current/selection/hover indexes holding freed
// `internalPointer()`s.
TEST_F(TableModelAdapterTest, AWholesaleChangeIsAResetNotALayoutChange) {
  QSignalSpy reset{&adapter_, &QAbstractItemModel::modelReset};
  QSignalSpy about_to_reset{&adapter_,
                            &QAbstractItemModel::modelAboutToBeReset};
  QSignalSpy layout{&adapter_, &QAbstractItemModel::layoutChanged};

  model_->ChangeWholesale();

  EXPECT_EQ(about_to_reset.count(), 1);
  EXPECT_EQ(reset.count(), 1);
  EXPECT_EQ(layout.count(), 0);
}

// A table has no second level; the root index in particular has column -1,
// which `flags()` used to pass straight into `columns_[...]`.
TEST_F(TableModelAdapterTest, AValidParentHasNoRowsOrColumns) {
  EXPECT_EQ(adapter_.rowCount(adapter_.index(0, 0)), 0);
  EXPECT_EQ(adapter_.columnCount(adapter_.index(0, 0)), 0);
  EXPECT_FALSE(adapter_.flags(QModelIndex{}) & Qt::ItemIsEditable);
}

TEST_F(TableModelAdapterTest, SatisfiesTheQtModelContractThroughMutations) {
  QAbstractItemModelTester tester{
      &adapter_, QAbstractItemModelTester::FailureReportingMode::Fatal};

  model_->AddItems(1, 2);
  model_->ChangeWholesale();
}

}  // namespace scada::aui
