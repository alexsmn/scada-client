#include "ui/qt/client_utils_qt.h"

#include "aui/models/simple_menu_model.h"
#include "aui/qt/image_util.h"
#include "aui/test/app_environment.h"
#include "resources/common_resources.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QColor>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QMenu>
#include <QPalette>
#include <QPixmap>
#include <QSize>
#include <QString>
#include <QStringList>
#include <gtest/gtest.h>

#include <array>
#include <filesystem>
#include <set>
#include <string>
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

// The set of glyphs actually shipped in res/client.qrc, read out of the
// compiled resource (client_qt_unittests links it: see app/qt/CMakeLists.txt).
QStringList ShippedGlyphResourcePaths() {
  QStringList paths;
  const QDir dir{QStringLiteral(":/icons")};
  for (const QString& name : dir.entryList(QStringList{QStringLiteral("*.svg")},
                                           QDir::Files, QDir::Name)) {
    paths.append(dir.filePath(name));
  }
  return paths;
}

// The fraction of the icon box carrying ink, as [0, 1]. A glyph lunasvg cannot
// draw returns 0 here while still yielding a perfectly valid, perfectly empty
// pixmap.
double InkCoverage(const QIcon& icon, int size) {
  const QImage image = icon.pixmap(QSize{size, size}, 1.0).toImage();
  if (image.isNull())
    return 0.0;
  int ink = 0;
  for (int y = 0; y < image.height(); ++y) {
    for (int x = 0; x < image.width(); ++x) {
      if (qAlpha(image.pixel(x, y)) > 128)
        ++ink;
    }
  }
  return static_cast<double>(ink) / (image.width() * image.height());
}

// Every shipped glyph must rasterise to something. The two qrc guards
// elsewhere (ConfigurationTreeGlyphs.EveryMappedGlyphIsShipped, and
// EveryToolbarIconLoadsAtTheRequestedSize above) prove a mapped path is
// registered and present; neither proves the bytes behind it *draw*. lunasvg
// returns a valid, empty bitmap for an element it does not support, so such a
// glyph renders blank with nothing failing — the regression class the
// rasteriser swap introduced the risk of, and the one §6 of
// docs/client/ux/iconography.md re-opens every time an icon is added.
//
// Driven off the resource directory rather than off kIconResources/kItemGlyphs
// so a glyph added to res/ is covered the day it lands: the hand-check this
// replaces was performed against 31 files and the set had already grown to 32.
//
// Deliberately not a golden-image test. The two rasterisers differ by up to
// 3.2% on identical input, so a pixel baseline cannot separate a lunasvg
// version bump from a real defect (backlog 156). Non-empty ink can.
TEST_F(ClientUtilsQtTest, EveryShippedGlyphRasterisesToInk) {
  const QStringList glyphs = ShippedGlyphResourcePaths();

  // Without this the loop below is vacuous — which is the exact failure shape
  // the test exists to catch, one level up.
  ASSERT_FALSE(glyphs.isEmpty())
      << ":/icons is empty; res/client.qrc is not linked into this binary";

  for (const QString& path : glyphs) {
    const QIcon icon =
        LoadTintedGlyph(path.toStdString(), 24, QColor{Qt::black});
    ASSERT_FALSE(icon.isNull()) << path.toStdString() << " did not load";
    EXPECT_GT(InkCoverage(icon, 24), 0.0)
        << path.toStdString()
        << " rasterises blank: lunasvg parsed it and drew nothing";
  }
}

// A glyph on disk that nobody listed in res/client.qrc is absent from the
// binary, so the test above would not see it and the UI would draw nothing for
// it. Compares the two sets rather than counting, so the failure names the
// file.
TEST_F(ClientUtilsQtTest, EveryGlyphOnDiskIsInTheResource) {
  // .../client/ui/qt/<this file>
  const std::filesystem::path icons_dir = std::filesystem::path{__FILE__}
                                              .parent_path()
                                              .parent_path()
                                              .parent_path() /
                                          "res" / "icons";
  ASSERT_TRUE(std::filesystem::is_directory(icons_dir)) << icons_dir.string();

  std::set<std::string> on_disk;
  for (const auto& entry : std::filesystem::directory_iterator{icons_dir}) {
    if (entry.path().extension() == ".svg")
      on_disk.insert(entry.path().filename().string());
  }

  std::set<std::string> in_resource;
  for (const QString& path : ShippedGlyphResourcePaths())
    in_resource.insert(QFileInfo{path}.fileName().toStdString());

  for (const std::string& name : on_disk) {
    EXPECT_TRUE(in_resource.contains(name))
        << name << " is in res/icons/ but not listed in res/client.qrc";
  }
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

// Radio rows used to be built exactly like check rows — setCheckable(true) and
// nothing else — so Settings → Style, Settings → Colour scheme, Page and
// Window all rendered as a column of check boxes with one ticked, reading as
// independent toggles rather than "pick one". Qt only draws the radio
// indicator for an action in an exclusive QActionGroup.
class BuildMenuRadioTest : public ::testing::Test {
 protected:
  // Counts the action groups BuildMenu parented to |menu|, which is how a
  // rebuild leak would show up.
  static int GroupCount(const QMenu& menu) {
    return static_cast<int>(
        menu.findChildren<QActionGroup*>(Qt::FindDirectChildrenOnly).size());
  }

  AppEnvironment app_env_;
  ReasonMenuModel::Delegate delegate_;
};

TEST_F(BuildMenuRadioTest, RadioItemsShareOneExclusiveGroup) {
  delegate_.enabled = true;
  ReasonMenuModel model{delegate_};
  model.AddRadioItem(1, u"Light", 0);
  model.AddRadioItem(2, u"Dark", 0);

  QMenu menu;
  BuildMenu(menu, model);

  ASSERT_EQ(menu.actions().size(), 2);
  QAction* first = menu.actions().at(0);
  QAction* second = menu.actions().at(1);
  ASSERT_NE(first->actionGroup(), nullptr);
  EXPECT_EQ(first->actionGroup(), second->actionGroup());
  EXPECT_TRUE(first->actionGroup()->isExclusive());
  EXPECT_TRUE(first->isCheckable());
  EXPECT_TRUE(second->isCheckable());
}

// A separator between radio rows does not start a new group: a rule drawn
// inside one choice is punctuation, not a second choice. Grouping is keyed by
// (model, group id) rather than by a run of adjacent rows, which is what makes
// that true. Colour scheme was the motivating case, with a rule between
// "Classic" and the themes, until Classic was removed on 2026-08-31.
TEST_F(BuildMenuRadioTest, ASeparatorDoesNotSplitTheGroup) {
  delegate_.enabled = true;
  ReasonMenuModel model{delegate_};
  model.AddRadioItem(1, u"Follow system", 0);
  model.AddSeparator(scada::aui::NORMAL_SEPARATOR);
  model.AddRadioItem(2, u"Dark", 0);

  QMenu menu;
  BuildMenu(menu, model);

  ASSERT_EQ(menu.actions().size(), 3);
  EXPECT_EQ(GroupCount(menu), 1);
  EXPECT_EQ(menu.actions().at(0)->actionGroup(),
            menu.actions().at(2)->actionGroup());
}

// Check rows keep their check box — the fix must not turn every toggle into a
// radio.
TEST_F(BuildMenuRadioTest, CheckItemsGetNoGroup) {
  delegate_.enabled = true;
  ReasonMenuModel model{delegate_};
  model.AddCheckItem(1, u"Show grid");

  QMenu menu;
  BuildMenu(menu, model);

  ASSERT_EQ(menu.actions().size(), 1);
  EXPECT_TRUE(menu.actions().front()->isCheckable());
  EXPECT_EQ(menu.actions().front()->actionGroup(), nullptr);
  EXPECT_EQ(GroupCount(menu), 0);
}

// An in-place submenu is merged into the same QMenu, so its group id 0 must
// not be confused with the host model's group id 0 — otherwise two unrelated
// choices become mutually exclusive.
TEST_F(BuildMenuRadioTest, AnInplaceSubmenuGetsItsOwnGroup) {
  delegate_.enabled = true;
  ReasonMenuModel host{delegate_};
  ReasonMenuModel inplace{delegate_};
  inplace.AddRadioItem(3, u"Metric", 0);
  host.AddRadioItem(1, u"Light", 0);
  host.AddInplaceMenu(&inplace);

  QMenu menu;
  BuildMenu(menu, host);

  ASSERT_EQ(menu.actions().size(), 2);
  ASSERT_NE(menu.actions().at(0)->actionGroup(), nullptr);
  ASSERT_NE(menu.actions().at(1)->actionGroup(), nullptr);
  EXPECT_NE(menu.actions().at(0)->actionGroup(),
            menu.actions().at(1)->actionGroup());
  EXPECT_EQ(GroupCount(menu), 2);
}

// The main menu rebuilds itself on every aboutToShow. QMenu::clear() deletes
// the actions but not the groups, so each build has to discard the previous
// one's rather than pile them up for the life of the menu.
TEST_F(BuildMenuRadioTest, RebuildingDoesNotAccumulateGroups) {
  delegate_.enabled = true;
  ReasonMenuModel model{delegate_};
  model.AddRadioItem(1, u"Light", 0);
  model.AddRadioItem(2, u"Dark", 0);

  QMenu menu;
  for (int i = 0; i < 3; ++i) {
    menu.clear();
    BuildMenu(menu, model);
    ASSERT_EQ(menu.actions().size(), 2);
    EXPECT_EQ(GroupCount(menu), 1) << "after build " << i;
  }
}

}  // namespace
