#include "shell_strip_capture.h"

#include "screenshot_config.h"
#include "widget_capture.h"

#include "aui/translation.h"
#include "main_window/activity_bar_qt.h"
#include "main_window/breadcrumb_qt.h"

#include <gtest/gtest.h>

#include <QAction>
#include <QWidgetAction>

#include <memory>
#include <vector>

namespace {

// A bar with no modes, so the capture is the zone the caller populates.
std::unique_ptr<ActivityBar> MakeEmptyRail() {
  return std::make_unique<ActivityBar>(nullptr, std::vector<ActivityBar::Mode>{},
                                       ActivityBar::ActivateCallback{});
}

}  // namespace

void SavePagesScreenshot(const ScreenshotSpec& spec) {
  std::unique_ptr<ActivityBar> rail = MakeEmptyRail();

  // Three pages, because a band of one button does not read as a band, and the
  // third is active so the marker is in the picture. Two carry an icon and one
  // does not: the fallback is the 1-based position, and showing both states is
  // the whole point of photographing this strip rather than describing it.
  rail->SetPages({
      {.page_id = 1, .title = Translate("Overview"), .icon_key = "overview"},
      {.page_id = 2, .title = Translate("Alarms"), .icon_key = "alarms"},
      {.page_id = 3, .title = Translate("Trends")},
  });
  rail->SetActivePage(3);

  SaveScreenshot(rail.get(), spec);
}

void SaveRailUtilitiesScreenshot(const ScreenshotSpec& spec) {
  std::unique_ptr<ActivityBar> rail = MakeEmptyRail();

  // An admin session: Users is gated, and a non-admin rail has Settings alone,
  // which would not show that this zone holds a set. The marker sits on Users
  // because a utility's marker means "that view is what the workspace is
  // showing" rather than "this utility was clicked".
  rail->SetUtilities(
      {
          {.utility_id = 1,
           .label = Translate("Settings"),
           .icon_kind = ActivityBar::Icon::kSettings},
          {.utility_id = 2,
           .label = Translate("Users"),
           .icon_kind = ActivityBar::Icon::kUsers},
      },
      ActivityBar::ActivateUtilityCallback{});
  rail->SetActiveUtility(2);

  // Hide the pages group's "+", which the constructor adds unconditionally and
  // exposes no accessor for. Without this the picture of the UTILITIES zone
  // leads with the add-page button, which belongs to the row next door.
  //
  // Found by what it IS rather than by what it says: every mode, page and
  // utility action is checkable — `MakeAction` sets that for all of them — and
  // the "+" is deliberately not, because "the '+' is an action, not a
  // destination — it must never carry a marker" (activity_bar_qt.cpp). So the
  // one non-separator, non-checkable action on the bar is the add-page button.
  //
  // The first attempt matched its tooltip and asserted the tooltip was safe
  // because the label is the glyph `+`. That was a misreading: `u"+"` is the
  // ICON label, and the tooltip is `Translate("New page")` — a translated
  // string, and exactly the kind of handle a capture must not use, since it
  // moves with the locale. The assertion below is what caught it.
  int hidden = 0;
  for (QAction* action : rail->actions()) {
    // Not the spacer: `addWidget` returns a QWidgetAction, and the rail's
    // expanding spacer is one. It is also non-separator and non-checkable, so
    // the first version of this handle hid it too and collapsed the layout
    // that pins the utilities to the foot — which the assertion caught as
    // `hidden == 2`.
    if (!action->isSeparator() && !action->isCheckable() &&
        qobject_cast<QWidgetAction*>(action) == nullptr) {
      action->setVisible(false);
      ++hidden;
    }
  }
  ASSERT_EQ(hidden, 1) << "expected exactly one non-checkable action on the "
                          "rail (the add-page '+'); the handle this capture "
                          "hides it by has stopped holding";

  SaveScreenshot(rail.get(), spec);
}

void SaveBreadcrumbScreenshot(const ScreenshotSpec& spec) {
  auto breadcrumb = std::make_unique<Breadcrumb>(nullptr);

  // Page / view / selection, the three-step path the shell builds. The ends
  // are `strong` and the middle is context, which is the rule the widget's own
  // header states and the reason a picture is worth more than the prose.
  const std::vector<Breadcrumb::Segment> segments = {
      {.label = QString::fromStdU16String(Translate("Overview")), .strong = true},
      {.label = QString::fromStdU16String(Translate("Graph"))},
      {.label = QString::fromStdU16String(Translate("Active power")),
       .strong = true},
  };
  breadcrumb->SetSegments(segments);

  SaveScreenshot(breadcrumb.get(), spec);
}
