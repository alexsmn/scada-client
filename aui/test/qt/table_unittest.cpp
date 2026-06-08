#include "aui/qt/table.h"

#include "aui/models/table_model.h"
#include "aui/test/qt/app_environment.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <gtest/gtest.h>

namespace {

class TestTableModel final : public aui::TableModel {
 public:
  int GetRowCount() override { return 2; }

  void GetCell(aui::TableCell& cell) override {
    cell.text = cell.row == 0 ? u"Default" : u"Explicit";
    if (cell.row == 1) {
      cell.text_color = aui::ColorCode::White;
      cell.cell_color = aui::ColorCode::Black;
    }
  }
};

std::vector<aui::TableColumn> MakeColumns() {
  return {{0, u"Name", 100, aui::TableColumn::LEFT}};
}

}  // namespace

TEST(TableTest, DefaultColorsUsePalette) {
  AppEnvironment app_env;

  aui::Table table{std::make_shared<TestTableModel>(), MakeColumns()};
  const auto index = table.model()->index(0, 0);

  EXPECT_FALSE(table.model()->data(index, Qt::ForegroundRole).isValid());
  EXPECT_FALSE(table.model()->data(index, Qt::BackgroundRole).isValid());
}

TEST(TableTest, DefaultItemPaletteUsesWindowThemeColors) {
  AppEnvironment app_env;

  QPalette palette = QApplication::palette();
  for (auto group :
       {QPalette::Active, QPalette::Inactive, QPalette::Disabled}) {
    palette.setColor(group, QPalette::Window, QColor{32, 33, 36});
    palette.setColor(group, QPalette::WindowText, QColor{232, 234, 237});
    palette.setColor(group, QPalette::Base, Qt::white);
    palette.setColor(group, QPalette::Text, Qt::black);
  }
  QApplication::setPalette(palette);

  aui::Table table{std::make_shared<TestTableModel>(), MakeColumns()};

  EXPECT_EQ(table.palette().color(QPalette::Base), QColor(32, 33, 36));
  EXPECT_EQ(table.palette().color(QPalette::Text), QColor(232, 234, 237));
  EXPECT_EQ(table.palette().color(QPalette::AlternateBase), QColor(32, 33, 36));
}

TEST(TableTest, ExplicitColorsOverridePalette) {
  AppEnvironment app_env;

  aui::Table table{std::make_shared<TestTableModel>(), MakeColumns()};
  const auto index = table.model()->index(1, 0);

  ASSERT_TRUE(table.model()->data(index, Qt::ForegroundRole).isValid());
  EXPECT_EQ(table.model()->data(index, Qt::ForegroundRole).value<QColor>(),
            QColor(Qt::white));
  ASSERT_TRUE(table.model()->data(index, Qt::BackgroundRole).isValid());
  EXPECT_EQ(table.model()->data(index, Qt::BackgroundRole).value<QColor>(),
            QColor(Qt::black));
}
