#include "aui/qt/grid_model_adapter.h"

#include "aui/models/grid_model.h"
#include "aui/models/grid_range.h"
#include "aui/models/header_model.h"
#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <QAbstractItemModelTester>
#include <QColor>
#include <QItemSelectionModel>
#include <QSignalSpy>
#include <QVariant>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace scada::aui {
namespace {

// A one-cell model whose colours the test controls (the row count comes from
// the row header model).
class StubGridModel : public GridModel {
 public:
  virtual void GetCell(GridCell& cell) override {
    ++get_cell_calls;
    cell.text = u"42";
    cell.text_color = text_color;
    cell.cell_color = cell_color;
    cell.alignment = alignment;
  }

  // The notification shapes a real model uses, exposed so the adapter tests
  // can drive them. The row count moves inside the guard, the way a real
  // model's own storage does.
  void AddRows(int first, int count) {
    ScopedRowsAdding adding{*this, first, count};
    row_count += count;
  }

  void RemoveRows(int first, int count) {
    ScopedRowsRemoving removing{*this, first, count};
    row_count -= count;
  }

  void ChangeWholesale() { NotifyModelChanged(); }
  void ChangeRange(const GridRange& range) { NotifyRangeChanged(range); }

  int row_count = 1;

  Color text_color = ColorCode::Transparent;
  Color cell_color = ColorCode::Transparent;
  std::optional<TableColumn::Alignment> alignment;
  int get_cell_calls = 0;
};

// Records the structural signals a QAbstractItemModel emits, in order, so a
// test can assert that the "about to" half was emitted at all -- which is the
// half `QItemSelectionModel` and `QSortFilterProxyModel` act on, and the half
// a bare `layoutChanged()` omits.
class SignalLog {
 public:
  explicit SignalLog(QAbstractItemModel& model) {
    Watch(model, &QAbstractItemModel::rowsAboutToBeInserted,
          "rowsAboutToBeInserted");
    Watch(model, &QAbstractItemModel::rowsInserted, "rowsInserted");
    Watch(model, &QAbstractItemModel::rowsAboutToBeRemoved,
          "rowsAboutToBeRemoved");
    Watch(model, &QAbstractItemModel::rowsRemoved, "rowsRemoved");
    QObject::connect(&model, &QAbstractItemModel::modelAboutToBeReset,
                     [this] { names_.push_back("modelAboutToBeReset"); });
    QObject::connect(&model, &QAbstractItemModel::modelReset,
                     [this] { names_.push_back("modelReset"); });
    QObject::connect(&model, &QAbstractItemModel::layoutAboutToBeChanged,
                     [this] { names_.push_back("layoutAboutToBeChanged"); });
    QObject::connect(&model, &QAbstractItemModel::layoutChanged,
                     [this] { names_.push_back("layoutChanged"); });
  }

  const std::vector<std::string>& names() const { return names_; }
  const std::vector<std::pair<int, int>>& ranges() const { return ranges_; }

 private:
  template <class Signal>
  void Watch(QAbstractItemModel& model, Signal signal, const char* name) {
    QObject::connect(&model, signal,
                     [this, name](const QModelIndex&, int first, int last) {
                       names_.push_back(name);
                       ranges_.emplace_back(first, last);
                     });
  }

  std::vector<std::string> names_;
  std::vector<std::pair<int, int>> ranges_;
};

// The row header, shaped like the real one. Every grid in the client uses
// `FixedRowModel`, which reads its count from the grid model live and emits
// nothing of its own -- so a row insertion reaches the adapter as the
// GridModel row signals and nothing else. A `ColumnHeaderModel` standing in
// for it would emit `ModelChanged` from `SetColumnCount` and land a reset
// inside the insert scope, which no real model does.
class StubRowModel : public HeaderModel {
 public:
  explicit StubRowModel(const StubGridModel& model) : model_{model} {}

  virtual int GetCount() const override { return model_.row_count; }
  virtual int GetSize(int index) const override { return 17; }
  virtual std::u16string GetTitle(int index) const override { return {}; }

 private:
  const StubGridModel& model_;
};

// A column header whose alignment the test controls.
class StubColumnModel : public ColumnHeaderModel {
 public:
  virtual TableColumn::Alignment GetAlignment(int index) const override {
    return alignment;
  }

  TableColumn::Alignment alignment = TableColumn::LEFT;
};

class GridModelAdapterTest : public testing::Test {
 protected:
  GridModelAdapterTest() {
    TableColumn column{0, u"C", 100};
    columns_->SetColumns(1, &column);
  }

  void TearDown() override { SetSeverityTheme(SeverityTheme::kDark); }

  QVariant Data(int role) { return adapter_.data(adapter_.index(0, 0), role); }

  AppEnvironment app_env_;
  std::shared_ptr<StubGridModel> model_ = std::make_shared<StubGridModel>();
  std::shared_ptr<StubRowModel> rows_ = std::make_shared<StubRowModel>(*model_);
  std::shared_ptr<StubColumnModel> columns_ =
      std::make_shared<StubColumnModel>();
  GridModelAdapter adapter_{model_, rows_, columns_};
};

// Unstyled cells fall through to the theme palette. Regression: the adapter
// used to answer hardcoded white/black defaults, which left every grid
// surface — transmission rules, node tables — white under a dark theme.
TEST_F(GridModelAdapterTest, UnstyledCellsFallThroughToThePalette) {
  EXPECT_FALSE(Data(Qt::ForegroundRole).isValid());
  EXPECT_FALSE(Data(Qt::BackgroundRole).isValid());
}

// Explicit colours always pass through.
TEST_F(GridModelAdapterTest, ExplicitColoursPassThrough) {
  model_->text_color = ColorCode::Red;
  model_->cell_color = ColorCode::Yellow;
  EXPECT_EQ(Data(Qt::ForegroundRole).value<QColor>(), QColor{Qt::red});
  EXPECT_EQ(Data(Qt::BackgroundRole).value<QColor>(), QColor{Qt::yellow});

  SetSeverityTheme(SeverityTheme::kDark);
  EXPECT_EQ(Data(Qt::ForegroundRole).value<QColor>(), QColor{Qt::red});
  EXPECT_EQ(Data(Qt::BackgroundRole).value<QColor>(), QColor{Qt::yellow});
}

// A themed cell with an explicit background but default text derives a
// contrasting text colour, so a semantically light cell (read-only grey,
// blink yellow) stays readable on the dark theme.
TEST_F(GridModelAdapterTest, ThemedExplicitBackgroundDerivesContrastingText) {
  SetSeverityTheme(SeverityTheme::kDark);

  model_->cell_color = ColorCode::Yellow;  // light
  EXPECT_EQ(Data(Qt::ForegroundRole).value<QColor>(), QColor{Qt::black});

  model_->cell_color = Rgba{0x20, 0x20, 0x20};  // dark
  EXPECT_EQ(Data(Qt::ForegroundRole).value<QColor>(), QColor{Qt::white});
}

// A column's alignment reaches Qt as Qt's own flags. Regression: the adapter
// returned the aui enum raw, and Qt read it as a flag mask — LEFT(0) became
// "no alignment" and RIGHT(1)/CENTER(2) became AlignLeft/AlignRight, so every
// grid in the client rendered left-aligned regardless of what its columns
// asked for.
TEST_F(GridModelAdapterTest, ColumnAlignmentReachesQtAsQtFlags) {
  columns_->alignment = TableColumn::RIGHT;
  EXPECT_EQ(Data(Qt::TextAlignmentRole).toInt(),
            static_cast<int>(Qt::AlignRight | Qt::AlignVCenter));

  columns_->alignment = TableColumn::CENTER;
  EXPECT_EQ(Data(Qt::TextAlignmentRole).toInt(),
            static_cast<int>(Qt::AlignHCenter | Qt::AlignVCenter));

  columns_->alignment = TableColumn::LEFT;
  EXPECT_EQ(Data(Qt::TextAlignmentRole).toInt(),
            static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter));
}

// A cell may override its column — how the spreadsheet's per-cell alignment
// reaches the screen. Regression: GridCell carried no alignment, so the
// sheet's stored formats were silently dropped at render time.
TEST_F(GridModelAdapterTest, CellAlignmentOverridesItsColumn) {
  columns_->alignment = TableColumn::LEFT;
  model_->alignment = TableColumn::RIGHT;
  EXPECT_EQ(Data(Qt::TextAlignmentRole).toInt(),
            static_cast<int>(Qt::AlignRight | Qt::AlignVCenter));
}

// A delegate asks for seven roles per paint and per sizeHint; the model's
// GetCell formats the value each time. Only the roles that read the cell may
// pay for it — the adapter used to fetch first and switch on the role after.
TEST_F(GridModelAdapterTest, RolesThatDoNotReadTheCellDoNotFetchIt) {
  Data(Qt::SizeHintRole);
  Data(Qt::CheckStateRole);
  Data(Qt::FontRole);
  Data(Qt::DecorationRole);
  EXPECT_EQ(model_->get_cell_calls, 0);

  Data(Qt::DisplayRole);
  EXPECT_EQ(model_->get_cell_calls, 1);
}

// --- Structural change notification (backlog 712) ---------------------------
//
// Everything below is one defect in four shapes: the adapter announced a
// structural change with a bare `layoutChanged()`. `QItemSelectionModel`
// early-returns without the matching `layoutAboutToBeChanged()`, so a
// selection keeps row numbers that now name different items -- and behind
// `TableProxyModel` (a QSortFilterProxyModel) the view's indexes keep freed
// `internalPointer()`s, because `_q_sourceLayoutChanged` deletes every mapping
// and remaps only what the "about to" half saved.

TEST_F(GridModelAdapterTest, AddingRowsAnnouncesAnInsertionNotALayoutChange) {
  SignalLog log{adapter_};

  model_->AddRows(1, 2);

  EXPECT_THAT(log.names(),
              testing::ElementsAre("rowsAboutToBeInserted", "rowsInserted"));
  EXPECT_THAT(log.ranges(), testing::Each(std::pair{1, 2}));
  EXPECT_EQ(adapter_.rowCount(), 3);
}

TEST_F(GridModelAdapterTest, RemovingRowsAnnouncesARemovalNotALayoutChange) {
  model_->AddRows(1, 2);
  SignalLog log{adapter_};

  model_->RemoveRows(1, 2);

  EXPECT_THAT(log.names(),
              testing::ElementsAre("rowsAboutToBeRemoved", "rowsRemoved"));
  EXPECT_THAT(log.ranges(), testing::Each(std::pair{1, 2}));
  EXPECT_EQ(adapter_.rowCount(), 1);
}

TEST_F(GridModelAdapterTest, AWholesaleChangeIsAResetNotALayoutChange) {
  SignalLog log{adapter_};

  model_->ChangeWholesale();

  EXPECT_THAT(log.names(),
              testing::ElementsAre("modelAboutToBeReset", "modelReset"));
}

// The selection is the reason the "about to" half matters. Told of a removal
// only afterwards, QItemSelectionModel keeps the row number it had -- so a
// selection on the last row survives a shrink and names a row that is gone.
TEST_F(GridModelAdapterTest, RemovingTheSelectedRowClearsTheSelection) {
  model_->AddRows(1, 2);
  QItemSelectionModel selection{&adapter_};
  selection.select(adapter_.index(2, 0), QItemSelectionModel::Select);
  ASSERT_TRUE(selection.hasSelection());

  model_->RemoveRows(2, 1);

  EXPECT_FALSE(selection.hasSelection());
}

// A ROWS range leaves column_count_ at 0, so reading all four GridRange fields
// regardless of type() made the bottom-right index(r, -1).
// QAbstractItemView::dataChanged takes its invalid-range branch on that: a
// full repaint AND updateEditorData() on every open editor, which re-sets the
// text from the model and clears isModified() -- so a push for an unrelated
// row silently discarded whatever the operator was typing.
TEST_F(GridModelAdapterTest, ARowsRangeIsBoundedToTheColumnCount) {
  model_->AddRows(1, 2);
  QSignalSpy spy{&adapter_, &QAbstractItemModel::dataChanged};

  model_->ChangeRange(GridRange::Rows(1, 1));

  ASSERT_EQ(spy.count(), 1);
  const QModelIndex top_left = spy.at(0).at(0).value<QModelIndex>();
  const QModelIndex bottom_right = spy.at(0).at(1).value<QModelIndex>();
  EXPECT_TRUE(top_left.isValid());
  EXPECT_TRUE(bottom_right.isValid());
  EXPECT_EQ(bottom_right.column(), adapter_.columnCount() - 1);
}

// An empty role list means "every role", which repaints and re-reads editors
// the change never touched. Naming the roles is the other half of the fix
// above.
TEST_F(GridModelAdapterTest, ARangeChangeNamesTheRolesItAffects) {
  QSignalSpy spy{&adapter_, &QAbstractItemModel::dataChanged};

  model_->ChangeRange(GridRange::Cell(0, 0));

  ASSERT_EQ(spy.count(), 1);
  const QList<int> roles = spy.at(0).at(2).value<QList<int>>();
  EXPECT_FALSE(roles.isEmpty());
  EXPECT_TRUE(roles.contains(Qt::DisplayRole));
  EXPECT_TRUE(roles.contains(Qt::BackgroundRole));
}

// HeaderModel::ModelChanged is the only signal AddColumn/DeleteColumn/
// SetColumns emit and it can change the section count -- which
// headerDataChanged cannot express, and which it rejects outright when
// logicalLast >= count(). The adapter passed GetCount() as logicalLast, so the
// title-only case never repainted and a shrink left the header at its old
// count.
TEST_F(GridModelAdapterTest, AChangedColumnCountIsAnnouncedStructurally) {
  SignalLog log{adapter_};

  TableColumn columns[] = {{0, u"A", 100}, {1, u"B", 100}};
  columns_->SetColumns(2, columns);

  EXPECT_THAT(log.names(),
              testing::ElementsAre("modelAboutToBeReset", "modelReset"));
  EXPECT_EQ(adapter_.columnCount(), 2);
}

TEST_F(GridModelAdapterTest, AnUnchangedColumnCountRepaintsTheTitles) {
  QSignalSpy spy{&adapter_, &QAbstractItemModel::headerDataChanged};

  TableColumn column{0, u"Renamed", 100};
  columns_->SetColumns(1, &column);

  ASSERT_EQ(spy.count(), 1);
  // logicalLast must be count() - 1: QHeaderView rejects the signal outright
  // when it is >= count(), which is how the rename used to reach nothing.
  EXPECT_EQ(spy.at(0).at(2).toInt(), 0);
  EXPECT_EQ(adapter_.headerData(0, Qt::Horizontal, Qt::DisplayRole).toString(),
            QStringLiteral("Renamed"));
}

// A shrunk header is still queried by section number: QHeaderView repaints
// from the count it last heard about. Out of range answers a default rather
// than indexing the vector.
TEST_F(GridModelAdapterTest, AHeaderSectionPastTheEndReadsAsEmpty) {
  EXPECT_EQ(columns_->GetTitle(5), std::u16string{});
  EXPECT_EQ(columns_->GetSize(5), 0);
}

// A table has no second level. Answering the top-level count for every parent
// claims an infinitely deep tree.
TEST_F(GridModelAdapterTest, AValidParentHasNoRowsOrColumns) {
  EXPECT_EQ(adapter_.rowCount(adapter_.index(0, 0)), 0);
  EXPECT_EQ(adapter_.columnCount(adapter_.index(0, 0)), 0);
}

// Qt's own conformance check over the whole contract: it asserts the
// begin/end pairing, the index/parent invariants and the row-count bookkeeping
// that the bespoke cases above only sample. Constructed in each test so its
// checks run against that test's mutations.
TEST_F(GridModelAdapterTest, SatisfiesTheQtModelContractThroughRowMutations) {
  QAbstractItemModelTester tester{
      &adapter_, QAbstractItemModelTester::FailureReportingMode::Fatal};

  model_->AddRows(1, 2);
  model_->RemoveRows(1, 1);
  model_->ChangeRange(GridRange::Rows(0, 1));
  model_->ChangeWholesale();
}

}  // namespace
}  // namespace scada::aui
