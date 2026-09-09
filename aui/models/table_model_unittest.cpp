#include "aui/models/table_model.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

namespace scada::aui {
namespace {

// A text-only model: each row is one string in column 0.
class TextTableModel : public TableModel {
 public:
  explicit TextTableModel(std::vector<std::u16string> rows)
      : rows_{std::move(rows)} {}

  int GetRowCount() override { return static_cast<int>(rows_.size()); }
  void GetCell(TableCell& cell) override { cell.text = rows_[cell.row]; }

 private:
  std::vector<std::u16string> rows_;
};

// The default cell comparison — inherited by every sortable column that does
// not override it — orders text for display: case-blind, in both alphabets,
// rather than by code point (task 716).
//
// The rows are chosen so ordinal comparison gets each pair *wrong*: every
// uppercase letter sorts below every lowercase one in code-point order, so a
// capitalised name jumped ahead of everything in lower case. A pair like
// "banana" against "Apple" would agree either way and pin nothing.
TEST(TableModelTest, DefaultCompareCellsIsCaseBlind) {
  TextTableModel model{{u"apple", u"Banana", u"cherry"}};
  EXPECT_LT(model.CompareCells(0, 1, 0), 0);  // apple < Banana
  EXPECT_GT(model.CompareCells(1, 0, 0), 0);  // Banana > apple
  EXPECT_LT(model.CompareCells(1, 2, 0), 0);  // Banana < cherry
  EXPECT_GT(model.CompareCells(2, 1, 0), 0);  // cherry > Banana
}

TEST(TableModelTest, DefaultCompareCellsOrdersCyrillicAlphabetically) {
  TextTableModel model{{u"Ёлка", u"яблоко", u"Ель", u"авария"}};
  EXPECT_LT(model.CompareCells(0, 1, 0), 0);  // Ёлка < яблоко
  EXPECT_GT(model.CompareCells(0, 2, 0), 0);  // Ёлка > Ель
  EXPECT_GT(model.CompareCells(0, 3, 0), 0);  // Ёлка > авария
  EXPECT_LT(model.CompareCells(3, 2, 0), 0);  // авария < Ель
}

}  // namespace
}  // namespace scada::aui
