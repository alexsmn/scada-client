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

// The complement of ValueLabels: the punctuation between the steps.
std::vector<QLabel*> SeparatorLabels(const Breadcrumb& breadcrumb) {
  std::vector<QLabel*> separators;
  for (QLabel* label : breadcrumb.findChildren<QLabel*>()) {
    if (label->toolTip().isEmpty())
      separators.push_back(label);
  }
  return separators;
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

// A view named after the object it is pointed at makes the last two steps the
// same word — a Graph tab on one series takes its title from that series, and
// the subject reads the same display name. Printing it twice states nothing
// twice, so the repeat collapses and the survivor keeps the subject's weight.
TEST_F(BreadcrumbTest, CollapsesAStepThatRepeatsTheOneBeforeIt) {
  Breadcrumb breadcrumb{nullptr};
  breadcrumb.resize(600, 24);
  const std::array segments = {
      Breadcrumb::Segment{.label = QStringLiteral("Page 1"), .strong = true},
      Breadcrumb::Segment{.label = QStringLiteral("Feeder current")},
      Breadcrumb::Segment{.label = QStringLiteral("Feeder current"),
                          .strong = true},
  };
  breadcrumb.SetSegments(segments);

  EXPECT_EQ(breadcrumb.Text(), QStringLiteral("Page 1 / Feeder current"));
  const std::vector<QLabel*> labels = ValueLabels(breadcrumb);
  ASSERT_EQ(labels.size(), 2u);
  EXPECT_TRUE(labels[1]->font().bold());
}

// Only a *neighbouring* repeat is noise. The same name at both ends of the path
// is a real path — a page named after the object a view is pointed at — and
// dropping either end would lose a step.
TEST_F(BreadcrumbTest, KeepsARepeatThatIsNotAdjacent) {
  Breadcrumb breadcrumb{nullptr};
  breadcrumb.resize(600, 24);
  const std::array segments = {
      Breadcrumb::Segment{.label = QStringLiteral("Feeder current"),
                          .strong = true},
      Breadcrumb::Segment{.label = QStringLiteral("Graph")},
      Breadcrumb::Segment{.label = QStringLiteral("Feeder current"),
                          .strong = true},
  };
  breadcrumb.SetSegments(segments);

  EXPECT_EQ(breadcrumb.Text(),
            QStringLiteral("Feeder current / Graph / Feeder current"));
  EXPECT_EQ(ValueLabels(breadcrumb).size(), 3u);
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

// Regression: the breadcrumb has to *ask* for the room its path needs.
//
// Every step label is `QSizePolicy::Ignored` — that is what stops their
// full-string hints becoming a floor the widget cannot shrink below — but an
// Ignored child contributes nothing to its parent's hint either. The widget's
// own hint therefore collapsed to the separators alone, a QToolBar handed it
// ~15px of a 1920px bar, every step elided away, and the context bar drew a
// bare `/ /`. Asserting against the separator-only width is the point: a hint
// that merely exists would pass a `> 0` check.
TEST_F(BreadcrumbTest, SizeHintAsksForTheWholePath) {
  Breadcrumb breadcrumb{nullptr};
  const std::array segments = {
      Breadcrumb::Segment{.label = QStringLiteral("Substation South"),
                          .strong = true},
      Breadcrumb::Segment{.label = QStringLiteral("Parameters")},
      Breadcrumb::Segment{.label = QStringLiteral("Feeder bay 12"),
                          .strong = true},
  };
  breadcrumb.SetSegments(segments);

  const QFontMetrics metrics{breadcrumb.font()};
  const int separators_only =
      metrics.horizontalAdvance(QStringLiteral(" / ")) * 2;
  EXPECT_GT(breadcrumb.sizeHint().width(), separators_only);
  // Bold steps are wider than the quiet measurement of the same text, so the
  // whole-path advance is a lower bound rather than the answer.
  EXPECT_GE(breadcrumb.sizeHint().width(),
            metrics.horizontalAdvance(breadcrumb.Text()));

  // And it tracks the path rather than being a constant.
  const int wide_hint = breadcrumb.sizeHint().width();
  const std::array shorter = {
      Breadcrumb::Segment{.label = QStringLiteral("A"), .strong = true},
  };
  breadcrumb.SetSegments(shorter);
  EXPECT_LT(breadcrumb.sizeHint().width(), wide_hint);
}

// sizeHint and the elision have to agree, or the breadcrumb asks for a width
// and then truncates inside it. Given exactly the room it requested, every step
// must render in full.
TEST_F(BreadcrumbTest, GivenTheWidthItAsksForNothingElides) {
  Breadcrumb breadcrumb{nullptr};
  const std::array segments = {
      Breadcrumb::Segment{.label = QStringLiteral("Page 1"), .strong = true},
      Breadcrumb::Segment{.label = QStringLiteral("Objects")},
      Breadcrumb::Segment{.label = QStringLiteral("Feeder bay 12"),
                          .strong = true},
  };
  breadcrumb.SetSegments(segments);
  breadcrumb.resize(breadcrumb.sizeHint());

  const std::vector<QLabel*> labels = ValueLabels(breadcrumb);
  ASSERT_EQ(labels.size(), 3u);
  for (std::size_t i = 0; i < labels.size(); ++i)
    EXPECT_EQ(labels[i]->text(), labels[i]->toolTip()) << "step " << i;
}

// Regression: below the useful width the whole slot goes empty, punctuation
// included. Blanking only the steps left the separators drawn — a bar saying
// `/ /` claims there is a path and then declines to name it, which is exactly
// the dangling separator this component promises never to render.
TEST_F(BreadcrumbTest, TooNarrowHidesThePunctuationWithTheSteps) {
  Breadcrumb breadcrumb{nullptr};
  const std::array segments = {
      Breadcrumb::Segment{.label = QStringLiteral("Substation South"),
                          .strong = true},
      Breadcrumb::Segment{.label = QStringLiteral("Parameters")},
      Breadcrumb::Segment{.label = QStringLiteral("Feeder bay 12"),
                          .strong = true},
  };
  breadcrumb.resize(1200, 24);
  breadcrumb.SetSegments(segments);

  const std::vector<QLabel*> separators = SeparatorLabels(breadcrumb);
  ASSERT_EQ(separators.size(), 2u);
  for (QLabel* separator : separators)
    EXPECT_FALSE(separator->isHidden());

  // `isHidden` rather than `isVisible`: the widget has no shown parent in a
  // unit test, so isVisible() is false either way and would assert nothing.
  breadcrumb.resize(24, 24);
  for (QLabel* label : ValueLabels(breadcrumb))
    EXPECT_TRUE(label->text().isEmpty());
  for (QLabel* separator : separators)
    EXPECT_TRUE(separator->isHidden());

  // And it comes back — the branch is a width response, not a one-way latch.
  breadcrumb.resize(1200, 24);
  for (QLabel* label : ValueLabels(breadcrumb))
    EXPECT_FALSE(label->text().isEmpty());
  for (QLabel* separator : separators)
    EXPECT_FALSE(separator->isHidden());
}

}  // namespace
