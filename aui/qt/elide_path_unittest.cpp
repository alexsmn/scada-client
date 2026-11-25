#include "aui/qt/elide_path.h"

#include "aui/test/app_environment.h"

#include <QFontMetrics>
#include <QGuiApplication>

#include <gtest/gtest.h>

namespace scada::aui {
namespace {

// Backlog 1126: the journal's Signal column must keep the signal's own name
// when the path does not fit, giving up the shared prefix first.
class ElidePathTest : public testing::Test {
 protected:
  int Width(const QString& text) const {
    return metrics_.horizontalAdvance(text);
  }

  AppEnvironment app_env_;
  QFontMetrics metrics_{QGuiApplication::font()};
  const QString separator_ = QStringLiteral(" : ");
  const QString path_ = QStringLiteral(
      "ЭСТРА-ПС : ТИ : "
      "Коэффициент "
      "мощности");
  const QString root_ = path_.section(separator_, 0, 0);
  const QString leaf_ = path_.section(separator_, -1);
  const QString ellipsis_{QChar{0x2026}};
};

TEST_F(ElidePathTest, APathThatFitsIsUnchanged) {
  EXPECT_EQ(ElidePath(path_, separator_, metrics_, Width(path_)), path_);
}

TEST_F(ElidePathTest, TheMiddleSegmentsGoFirst) {
  const QString expected = root_ + separator_ + ellipsis_ + separator_ + leaf_;
  EXPECT_EQ(ElidePath(path_, separator_, metrics_, Width(expected)), expected);
}

TEST_F(ElidePathTest, ThenTheRoot) {
  const QString expected = ellipsis_ + separator_ + leaf_;
  EXPECT_EQ(ElidePath(path_, separator_, metrics_, Width(expected)), expected);
}

TEST_F(ElidePathTest, OnlyThenTheLeafItselfKeepingItsStart) {
  const int width = Width(leaf_) / 2;
  const QString elided = ElidePath(path_, separator_, metrics_, width);
  EXPECT_EQ(elided, metrics_.elidedText(leaf_, Qt::ElideRight, width));
  EXPECT_TRUE(elided.startsWith(leaf_.left(1)));
}

// A plain name has no prefix to give up.
TEST_F(ElidePathTest, ANameWithoutSeparatorsElidesAtItsEnd) {
  const QString name = QStringLiteral("Temperature of transformer T1");
  const int width = Width(name) / 2;
  EXPECT_EQ(ElidePath(name, separator_, metrics_, width),
            metrics_.elidedText(name, Qt::ElideRight, width));
}

}  // namespace
}  // namespace scada::aui
