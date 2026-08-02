#include "main_window/breadcrumb_qt.h"

#include "aui/test/app_environment.h"

#include <QApplication>
#include <QFontMetrics>
#include <QLabel>

#include <array>
#include <vector>

#include <gtest/gtest.h>

namespace {

std::vector<QLabel*> ValueLabels(const Breadcrumb& breadcrumb) {
  std::vector<QLabel*> labels;
  for (QLabel* label : breadcrumb.findChildren<QLabel*>()) {
    // Separators carry no tooltip; value labels always do (it is the
    // untruncated step).
    if (!label->toolTip().isEmpty())
      labels.push_back(label);
  }
  return labels;
}

class BreadcrumbTest : public ::testing::Test {
 protected:
  // Per-test QApplication; see ActivityBarTest for why it is never static.
  AppEnvironment app_env_;
};

TEST_F(BreadcrumbTest, EmptyPathHidesTheWidget) {
  Breadcrumb breadcrumb{nullptr};
  breadcrumb.SetSegments({});
  EXPECT_EQ(breadcrumb.Text(), QString{});
  EXPECT_FALSE(breadcrumb.isVisible());
}

TEST_F(BreadcrumbTest, JoinsStepsWithSeparators) {
  Breadcrumb breadcrumb{nullptr};
  breadcrumb.resize(600, 24);
  const std::array segments = {
      Breadcrumb::Segment{.label = QStringLiteral("Page 1"), .strong = true},
      Breadcrumb::Segment{.label = QStringLiteral("Parameters")},
      Breadcrumb::Segment{.label = QStringLiteral("KP-02"), .strong = true},
  };
  breadcrumb.SetSegments(segments);
  EXPECT_EQ(breadcrumb.Text(), QStringLiteral("Page 1 / Parameters / KP-02"));
}

// A view with no selection has no subject. The empty step has to disappear
// entirely rather than leave a dangling separator, which is what makes a
// fixed-arity path safe for the caller to build.
TEST_F(BreadcrumbTest, DropsEmptySteps) {
  Breadcrumb breadcrumb{nullptr};
  breadcrumb.resize(600, 24);
  const std::array segments = {
      Breadcrumb::Segment{.label = QStringLiteral("Page 1"), .strong = true},
      Breadcrumb::Segment{.label = QStringLiteral("Graph")},
      Breadcrumb::Segment{.label = QString{}, .strong = true},
  };
  breadcrumb.SetSegments(segments);
  EXPECT_EQ(breadcrumb.Text(), QStringLiteral("Page 1 / Graph"));
  EXPECT_EQ(ValueLabels(breadcrumb).size(), 2u);
}

// Emphasis is a font weight taken from the widget's own font, never a colour
// or a stylesheet — that is what lets the OS theme and text-size setting reach
// it (shell.md §9).
TEST_F(BreadcrumbTest, EmphasisIsAFontWeightNotAColour) {
  Breadcrumb breadcrumb{nullptr};
  breadcrumb.resize(600, 24);
  const std::array segments = {
      Breadcrumb::Segment{.label = QStringLiteral("Page 1"), .strong = true},
      Breadcrumb::Segment{.label = QStringLiteral("Graph")},
  };
  breadcrumb.SetSegments(segments);

  const std::vector<QLabel*> labels = ValueLabels(breadcrumb);
  ASSERT_EQ(labels.size(), 2u);
  EXPECT_TRUE(labels[0]->font().bold());
  EXPECT_FALSE(labels[1]->font().bold());
  // The quiet step is de-emphasised through a palette role, so a theme change
  // repaints it with no code.
  EXPECT_EQ(labels[1]->foregroundRole(), QPalette::PlaceholderText);
  EXPECT_TRUE(breadcrumb.styleSheet().isEmpty());
}

// The untruncated step stays reachable as a tooltip, so elision never costs the
// operator the name.
TEST_F(BreadcrumbTest, KeepsTheFullLabelAsATooltip) {
  Breadcrumb breadcrumb{nullptr};
  breadcrumb.resize(600, 24);
  const QString long_name =
      QStringLiteral("Substation South feeder bay 12 disconnector");
  const std::array segments = {
      Breadcrumb::Segment{.label = long_name, .strong = true},
  };
  breadcrumb.SetSegments(segments);

  const std::vector<QLabel*> labels = ValueLabels(breadcrumb);
  ASSERT_EQ(labels.size(), 1u);
  EXPECT_EQ(labels[0]->toolTip(), long_name);
}

// Elision is computed against the width the layout actually grants, not a fixed
// pixel budget — the rule CommandField established. A narrow bar must shorten
// the text rather than overflow it.
TEST_F(BreadcrumbTest, ElidesAgainstTheGrantedWidth) {
  Breadcrumb breadcrumb{nullptr};
  const QString long_name =
      QStringLiteral("Substation South feeder bay 12 disconnector");
  const std::array segments = {
      Breadcrumb::Segment{.label = long_name, .strong = true},
  };

  breadcrumb.resize(1200, 24);
  breadcrumb.SetSegments(segments);
  const std::vector<QLabel*> wide = ValueLabels(breadcrumb);
  ASSERT_EQ(wide.size(), 1u);
  const QString wide_text = wide[0]->text();

  breadcrumb.resize(160, 24);
  const std::vector<QLabel*> narrow = ValueLabels(breadcrumb);
  ASSERT_EQ(narrow.size(), 1u);
  const QString narrow_text = narrow[0]->text();

  EXPECT_EQ(wide_text, long_name);
  EXPECT_NE(narrow_text, long_name);
  EXPECT_LT(narrow_text.size(), wide_text.size());
  // Whatever survives must fit, or elision has not done its job.
  const QFontMetrics metrics{narrow[0]->font()};
  EXPECT_LE(metrics.horizontalAdvance(narrow_text), 160);
}

}  // namespace
