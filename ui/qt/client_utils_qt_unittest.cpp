#include "ui/qt/client_utils_qt.h"

#include "aui/models/simple_menu_model.h"
#include "aui/test/app_environment.h"
#include "resources/common_resources.h"

#include <QAction>
#include <QPalette>
#include <QImage>
#include <QApplication>
#include <QMenu>
#include <QPixmap>
#include <QSize>
#include <gtest/gtest.h>

#include <array>
#include <utility>

namespace {

// The command/action resource ids that modules attach to toolbar actions via
// Action::image_id_. Every one must resolve to an icon through LoadPixmap on
// all platforms; a blank toolbar on macOS/Linux was the regression this
// guards against. Keep in lock-step with the table in client_utils_qt.cpp and
// res/client.qrc. (ID_APPLICATION is excluded: it is the About-dialog
// application icon, asked for at a different size.)
constexpr std::array<std::pair<const char*, unsigned>, 16> kToolbarIcons{{
    {"ID_GRAPH_VIEW", ID_GRAPH_VIEW},
    {"ID_MODUS_VIEW", ID_MODUS_VIEW},
    {"ID_TABLE_VIEW", ID_TABLE_VIEW},
    {"IDB_SUMMARY", IDB_SUMMARY},
    {"ID_EVENT_VIEW", ID_EVENT_VIEW},
    {"IDB_OPEN_EVENTS", IDB_OPEN_EVENTS},
    {"IDB_TIMED_DATA", IDB_TIMED_DATA},
    {"IDB_RECORD_EDITOR", IDB_RECORD_EDITOR},
    {"IDB_PRINTER", IDB_PRINTER},
    {"IDB_WRITE", IDB_WRITE},
    {"IDB_WRITE_MANUAL", IDB_WRITE_MANUAL},
    {"IDB_UNLOCK", IDB_UNLOCK},
    {"IDB_COPY", IDB_COPY},
    {"IDB_PASTE", IDB_PASTE},
    {"IDB_DELETE", IDB_DELETE},
    {"IDB_ACKNOWLEDGE_ALL", IDB_ACKNOWLEDGE_ALL},
}};

class ClientUtilsQtTest : public testing::Test {
 protected:
  AppEnvironment app_env_;  // Constructs a QApplication for QPixmap loading.
};

TEST_F(ClientUtilsQtTest, EveryToolbarIconLoadsAtTheRequestedSize) {
  for (const auto& [name, id] : kToolbarIcons) {
    QPixmap pixmap = LoadPixmap(id, 24);
    ASSERT_FALSE(pixmap.isNull())
        << name << " (id " << id << ") did not resolve to an icon; check "
        << "res/client.qrc and the table in client_utils_qt.cpp";
    EXPECT_EQ(pixmap.deviceIndependentSize(), QSizeF(24, 24)) << name;
  }
}

// The glyphs are vector, so a bigger request means more pixels rather than an
// upscaled 16 px raster — the whole point of leaving the bitmaps behind. This
// is what the old "must be exactly 16x16" assertion becomes.
TEST_F(ClientUtilsQtTest, IconsAreRenderedNotUpscaled) {
  const QPixmap small = LoadPixmap(IDB_COPY, 16);
  const QPixmap large = LoadPixmap(IDB_COPY, 64);

  ASSERT_FALSE(small.isNull());
  ASSERT_FALSE(large.isNull());
  EXPECT_GT(large.width(), small.width());
}

// The files carry stroke="currentColor", which Qt resolves to black. Without
// tinting every icon would be a black-on-dark silhouette.
TEST_F(ClientUtilsQtTest, IconsAreTintedFromThePalette) {
  const QImage image = LoadPixmap(IDB_COPY, 24).toImage();

  const QColor expected = QApplication::palette().color(QPalette::WindowText);
  bool found = false;
  for (int y = 0; y < image.height() && !found; ++y) {
    for (int x = 0; x < image.width(); ++x) {
      const QColor pixel = image.pixelColor(x, y);
      if (pixel.alpha() > 200) {
        EXPECT_EQ(pixel.red(), expected.red());
        EXPECT_EQ(pixel.green(), expected.green());
        EXPECT_EQ(pixel.blue(), expected.blue());
        found = true;
        break;
      }
    }
  }
  EXPECT_TRUE(found) << "nothing was drawn";
}

TEST_F(ClientUtilsQtTest, ApplicationIconLoads) {
  // The About-dialog icon (about_dialog.cpp) also flows through LoadPixmap,
  // at the header size rather than the toolbar one.
  QPixmap pixmap = LoadPixmap(ID_APPLICATION, 64);
  ASSERT_FALSE(pixmap.isNull());
  EXPECT_EQ(pixmap.deviceIndependentSize(), QSizeF(64, 64));
}

TEST_F(ClientUtilsQtTest, UnmappedIdsReturnNullPixmap) {
  EXPECT_TRUE(LoadPixmap(0).isNull());
  EXPECT_TRUE(LoadPixmap(0xFFFFFFFFu).isNull());
}

// A menu model whose items' enablement and reason the test controls.
class ReasonMenuModel : public scada::aui::SimpleMenuModel {
 public:
  class Delegate : public scada::aui::SimpleMenuModel::Delegate {
   public:
    virtual bool IsCommandIdChecked(int command_id) const override {
      return false;
    }
    virtual bool IsCommandIdEnabled(int command_id) const override {
      return enabled;
    }
    virtual std::u16string GetDisabledReasonForCommandId(
        int command_id) const override {
      return reason;
    }
    virtual void ExecuteCommand(int command_id) override {}

    bool enabled = false;
    std::u16string reason;
  };

  explicit ReasonMenuModel(Delegate& delegate) : SimpleMenuModel{&delegate} {}
};

class BuildMenuReasonTest : public ::testing::Test {
 protected:
  AppEnvironment app_env_;
  ReasonMenuModel::Delegate delegate_;
};

// A disabled entry carries its reason as a tooltip, and the menu opts into
// showing tooltips — Qt hides them otherwise, and hides them for disabled
// items unless the menu asks.
TEST_F(BuildMenuReasonTest, DisabledEntryShowsItsReason) {
  delegate_.enabled = false;
  delegate_.reason = u"no output channel";
  ReasonMenuModel model{delegate_};
  model.AddItem(1, u"Control…");

  QMenu menu;
  BuildMenu(menu, model);

  ASSERT_EQ(menu.actions().size(), 1);
  QAction* action = menu.actions().front();
  EXPECT_FALSE(action->isEnabled());
  EXPECT_EQ(action->toolTip(), QStringLiteral("no output channel"));
  EXPECT_TRUE(menu.toolTipsVisible());
}

// An enabled entry is never asked for a reason and shows no tooltip, so the
// hover text stays meaningful.
TEST_F(BuildMenuReasonTest, EnabledEntryHasNoReasonTooltip) {
  delegate_.enabled = true;
  delegate_.reason = u"never shown";
  ReasonMenuModel model{delegate_};
  model.AddItem(1, u"Control…");

  QMenu menu;
  BuildMenu(menu, model);

  ASSERT_EQ(menu.actions().size(), 1);
  QAction* action = menu.actions().front();
  EXPECT_TRUE(action->isEnabled());
  // Qt defaults an action's tooltip to its text; what matters is that the
  // reason was not adopted and the menu did not opt into tooltips.
  EXPECT_NE(action->toolTip(), QStringLiteral("never shown"));
  EXPECT_FALSE(menu.toolTipsVisible());
}

// A disabled entry with nothing to say leaves the menu alone rather than
// showing an empty tooltip.
TEST_F(BuildMenuReasonTest, DisabledEntryWithoutReasonShowsNoTooltip) {
  delegate_.enabled = false;
  delegate_.reason.clear();
  ReasonMenuModel model{delegate_};
  model.AddItem(1, u"Control…");

  QMenu menu;
  BuildMenu(menu, model);

  ASSERT_EQ(menu.actions().size(), 1);
  EXPECT_FALSE(menu.actions().front()->isEnabled());
  EXPECT_FALSE(menu.toolTipsVisible());
}

}  // namespace
