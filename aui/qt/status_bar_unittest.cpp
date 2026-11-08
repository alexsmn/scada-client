#include "aui/qt/status_bar.h"

#include "aui/models/status_bar_model_impl.h"
#include "aui/test/app_environment.h"

#include <QLabel>

#include <gtest/gtest.h>

#include <string>

namespace scada::aui {
namespace {

// A pane that opts into hiding takes no room while empty, so a rarely shown
// warning cannot shift the panes beside it; a pane that does not opt in keeps
// its place as before.
TEST(StatusBarTest, APaneHiddenWhenEmptyShowsOnlyWithText) {
  AppEnvironment app_env;

  std::u16string notice;
  StatusBarModelImpl model;
  model.AddPane({.text_provider = [] { return std::u16string{}; }});
  // Both panes are labels added after QStatusBar's own children, so they are
  // the last two in creation order.
  const int notice_index =
      model.AddPane({.text_provider = [&notice] { return notice; },
                     .hidden_when_empty = true});

  StatusBar status_bar{model};
  const QList<QLabel*> panes =
      status_bar.findChildren<QLabel*>(Qt::FindDirectChildrenOnly);
  ASSERT_GE(panes.size(), 2);
  QLabel* const plain = panes[panes.size() - 2];
  QLabel* const notice_pane = panes.back();

  EXPECT_FALSE(plain->isHidden());
  EXPECT_TRUE(notice_pane->isHidden());

  notice = u"warning";
  model.NotifyPanesChanged(notice_index);
  EXPECT_FALSE(notice_pane->isHidden());
  EXPECT_EQ(notice_pane->text(), QStringLiteral("warning"));

  notice.clear();
  model.NotifyPanesChanged(notice_index);
  EXPECT_TRUE(notice_pane->isHidden());
  EXPECT_FALSE(plain->isHidden());
}

}  // namespace
}  // namespace scada::aui
