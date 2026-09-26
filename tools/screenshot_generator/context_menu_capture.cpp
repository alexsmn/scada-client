// Context-menu captures (the `auto-menu` manifest tag): the right-click menus
// of the object tree, the hardware tree and the configuration grid.
//
// These drive a REAL right-click rather than assembling a menu from the
// command registries. A `QContextMenuEvent` reaches the aui widget, whose
// `Qt::CustomContextMenu` policy emits `customContextMenuRequested`, whose
// handler calls `ControllerDelegate::ShowPopupMenu` — so the menu is built by
// the one code path the operator's own right-click uses, and a published image
// cannot document a menu nobody sees.
//
// What stops the menu from blocking is `MainWindow::SetPopupMenuInterceptor`,
// which diverts the built menu to a callback instead of `exec()`ing it;
// `QMenu::exec` runs a nested event loop and offscreen nothing would dismiss
// it. Everything before that point is untouched, which is what makes these
// faithful: the view that was clicked supplies its own `merge_menu` (the
// grid's command set is not the tree's), and `BuildMenu` calls
// `MenuWillShow()`, so the disabled rows several of these images show are
// resolved against the live selection rather than assumed.
//
// Like the two menu-bar captures beside them these carry no
// `screenshot_data.json` spec — they drive the live widget tree — so
// `check_screenshots.py` verifies no dimensions for them and the content
// assertions below are the whole regression net. See "auto-menu — a menu
// popup" in docs/ops/client-screenshots.md.

#include "explorer_selection.h"
#include "publish_guard.h"
#include "screenshot_config.h"
#include "screenshot_fixture.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "view_capture.h"
#include "widget_capture.h"

#include "app/client_application.h"
#include "aui/qt/tree.h"
#include "aui/translation.h"
#include "controller/window_info.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "profile/profile.h"
#include "profile/window_definition.h"

#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QImage>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QLatin1StringView>
#include <QMainWindow>
#include <QMenu>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QStringList>
#include <QTableView>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <functional>
#include <span>
#include <string>

namespace {

using scada::screenshot_generator::FindTreeRowByPath;
using scada::screenshot_generator::FindTreeWidget;
using scada::screenshot_generator::MaterializeExplorerTree;
using scada::screenshot_generator::PumpEventLoopFor;
using scada::screenshot_generator::ScreenshotGenerator;
using scada::screenshot_generator::ShowMainWindowForMenuCapture;
using scada::screenshot_generator::WaitForAwaitable;
using scada::screenshot_generator::WaitForPendingNodeLoads;
using scada::screenshot_generator::WaitUntil;

// Qt's own object name for a scroll area's viewport, which is how the
// right-click below finds the widget a real one would land on.
constexpr QLatin1StringView kScrollAreaViewportName{"qt_scrollarea_viewport"};

void PumpEvents(int iterations = 10) {
  for (int i = 0; i < iterations; ++i)
    QApplication::processEvents();
}

// The title a row carries, looked up through `Translate` rather than written
// as its Russian text: the client's own catalog is the stable key, and the
// manual's originals were captured under an older vocabulary (they say
// «Параметры» where the client now says «Свойства»).
QString RowTitle(const char* english_source) {
  return QString::fromStdU16String(Translate(english_source));
}

// The non-separator rows of `menu`, in order. A menu of nothing but separators
// would otherwise read as populated.
QStringList RowTitles(const QMenu& menu) {
  QStringList titles;
  for (const QAction* action : menu.actions()) {
    if (!action->isSeparator())
      titles << action->text();
  }
  return titles;
}

// The rows `menu` renders greyed out. Several of the manual's images are about
// exactly this state, so it is asserted rather than left to the pixels.
QStringList DisabledRowTitles(const QMenu& menu) {
  QStringList titles;
  for (const QAction* action : menu.actions()) {
    if (!action->isSeparator() && !action->isEnabled())
      titles << action->text();
  }
  return titles;
}

QAction* FindRow(QMenu& menu, const QString& title) {
  for (QAction* action : menu.actions()) {
    if (!action->isSeparator() && action->text() == title)
      return action;
  }
  return nullptr;
}

// Lays `menu` out at its natural size and grabs it. A `QMenu` renders without
// ever being popped up, which is what lets these run offscreen at all.
QPixmap GrabMenu(QMenu& menu) {
  menu.ensurePolished();
  menu.adjustSize();
  PumpEvents();
  return GrabWhenSettled(&menu);
}

// Draws `submenu` beside `parent` where Qt would pop it up: butted against the
// parent's right frame, its first row level with the row that opens it.
//
// This is a composite rather than a grab, and it has to be. An open submenu is
// a SECOND top-level popup window, so no single `grab()` reaches both — and
// offscreen there is no screen to grab instead. Both halves are real renders
// of the real widgets; only their relative placement is computed here, from
// the parent's own action geometry and frame width, so it lands where a real
// popup lands. The alternative was to publish these two with the submenu shut,
// which is the one thing their pages are about.
QPixmap CompositeWithSubmenu(QMenu& parent, QAction& row, QMenu& submenu) {
  const QPixmap parent_pixmap = GrabMenu(parent);
  const QPixmap submenu_pixmap = GrabMenu(submenu);

  // The panel inset, i.e. how far the frame sits from the widget edge. The
  // submenu overlaps it, which is why a real pair reads as one shape.
  const int frame = parent.contentsRect().left();
  const QRect row_rect = parent.actionGeometry(&row);
  const int x = parent_pixmap.width() - frame;
  const int y = std::max(0, row_rect.top() - frame);

  QImage composed{
      QSize{x + submenu_pixmap.width(),
            std::max(parent_pixmap.height(), y + submenu_pixmap.height())},
      QImage::Format_ARGB32_Premultiplied};
  composed.fill(Qt::transparent);
  {
    QPainter painter{&composed};
    painter.drawPixmap(0, 0, parent_pixmap);
    painter.drawPixmap(x, y, submenu_pixmap);
  }
  return QPixmap::fromImage(composed);
}

// Delivers a right-click to `target`.
//
// The event goes to the VIEWPORT, not to the tree or grid itself, which is
// where a real one lands and the only place it works:
// `QAbstractScrollArea::event` ignores a mouse-reason `QEvent::ContextMenu`
// addressed to the scroll area, so the same event sent to `target` reaches no
// handler at all and a capture would silently render nothing. (Measured
// against this tree's Qt, 2026-09-26: `contextMenuPolicy` is
// `Qt::CustomContextMenu` on the widget, and only the viewport delivery
// produced a menu.) From there Qt emits `customContextMenuRequested`, whose
// handler is the view's own — so the rest of the path is the operator's.
void SendContextMenuClick(QWidget& target) {
  QWidget* viewport = target.findChild<QWidget*>(kScrollAreaViewportName);
  if (!viewport) {
    ADD_FAILURE() << "the widget under the right-click has no scroll-area "
                     "viewport to deliver a context-menu event to";
    return;
  }
  const QPoint local{viewport->width() / 2, viewport->height() / 2};
  QContextMenuEvent event{QContextMenuEvent::Mouse, local,
                          viewport->mapToGlobal(local)};
  QApplication::sendEvent(viewport, &event);
  PumpEvents();
}

// What one capture asked of the menu it opened, and what it got back.
struct ContextMenuRender {
  // Set when the interceptor ran at all. False means the right-click never
  // reached `ShowPopupMenu`, which is a wiring failure rather than an empty
  // menu.
  bool opened = false;
  QPixmap pixmap;
  QStringList rows;
  QStringList disabled_rows;
  QStringList submenu_rows;
};

// Fires a right-click on `target` and renders the context menu it opens.
//
// `highlighted_row` is drawn as the active row, the way the manual's images
// show the command they are about; `open_submenu` is composited beside the
// menu. Both name the English catalog source of the row, and both are
// optional.
ContextMenuRender OpenContextMenu(MainWindow& main_window,
                                  QWidget& target,
                                  const char* highlighted_row,
                                  const char* open_submenu) {
  ContextMenuRender render;

  main_window.SetPopupMenuInterceptor([&](QMenu& menu) {
    render.opened = true;
    render.rows = RowTitles(menu);
    render.disabled_rows = DisabledRowTitles(menu);

    if (highlighted_row) {
      QAction* row = FindRow(menu, RowTitle(highlighted_row));
      if (!row) {
        ADD_FAILURE() << "no " << highlighted_row << " row in the menu: "
                      << render.rows.join(QLatin1String(" | ")).toStdString();
      } else {
        // Renders the row selected, which is what a hovered row looks like.
        // The menu is never shown, so nothing competes for the active row.
        menu.setActiveAction(row);
      }
    }

    if (!open_submenu) {
      render.pixmap = GrabMenu(menu);
      return;
    }

    QAction* submenu_row = FindRow(menu, RowTitle(open_submenu));
    if (!submenu_row || !submenu_row->menu()) {
      ADD_FAILURE() << "no " << open_submenu << " submenu in the menu: "
                    << render.rows.join(QLatin1String(" | ")).toStdString();
      render.pixmap = GrabMenu(menu);
      return;
    }

    QMenu& submenu = *submenu_row->menu();
    // A real hover rebuilds the submenu from its model before showing it
    // (`BuildMenu` connects that to `aboutToShow`), so the capture does the
    // same rather than publishing whatever the eager first build left.
    emit submenu.aboutToShow();
    PumpEvents();
    render.submenu_rows = RowTitles(submenu);
    render.pixmap = CompositeWithSubmenu(menu, *submenu_row, submenu);
  });

  SendContextMenuClick(target);

  main_window.SetPopupMenuInterceptor({});
  return render;
}

// Opens the context menu on `target` and activates `row`, the way a click on
// that row does. Used to reach a surface the operator reaches through a menu,
// rather than assembling it from a saved page.
bool TriggerContextMenuRow(MainWindow& main_window,
                           QWidget& target,
                           const char* row_source) {
  bool triggered = false;
  main_window.SetPopupMenuInterceptor([&](QMenu& menu) {
    if (QAction* row = FindRow(menu, RowTitle(row_source))) {
      row->trigger();
      triggered = true;
    } else {
      ADD_FAILURE() << "no " << row_source << " row to activate: "
                    << RowTitles(menu).join(QLatin1String(" | ")).toStdString();
    }
  });
  SendContextMenuClick(target);
  main_window.SetPopupMenuInterceptor({});
  PumpEvents(20);
  return triggered;
}

void SaveMenuPixmap(const QPixmap& pixmap,
                    const char* filename,
                    const CapturePublishGuard& publish_guard) {
  const auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);

  if (!publish_guard.ShouldPublish())
    return;

  ASSERT_FALSE(pixmap.isNull()) << filename << " grabbed an empty pixmap";
  ASSERT_TRUE(
      pixmap.save(QString::fromStdString(OutputPathFor(filename).string())))
      << "could not write " << filename;
}

// A tree the capture can right-click in: the named window's view, materialized
// and activated. Returns null (after reporting) when the page did not open it.
struct ExplorerTarget {
  scada::aui::Tree* tree = nullptr;
  OpenedView* view = nullptr;
};

ExplorerTarget OpenExplorerTree(ClientApplication& app,
                                MainWindow& main_window,
                                AnyExecutor executor,
                                std::string_view window_type) {
  // Select the owning activity-rail mode FIRST. A pane mode owns a set of
  // pane types and closes the ones outside it, so a pane named only by the
  // saved page can be shut again before the capture looks for it — which is
  // what the hardware tree does, and it presents as "no view opened" rather
  // than as an empty one.
  main_window.SelectPaneModeForPane(window_type);
  PumpEvents(20);

  ExplorerTarget target;
  WaitUntil([&] {
    for (OpenedView* view : main_window.opened_views()) {
      if (view->window_info().name != window_type)
        continue;
      target.view = view;
      target.tree = FindTreeWidget(view->view());
      return target.tree != nullptr;
    }
    return false;
  });
  if (!target.tree) {
    ADD_FAILURE() << "no " << window_type << " tree view opened";
    return {};
  }

  main_window.ActivateView(*target.view);
  PumpEvents(20);

  if (!MaterializeExplorerTree(*target.tree, nullptr, executor,
                               app.node_service())) {
    return {};
  }
  return target;
}

// Selects `path` in `tree` and waits for the selection to reach the shell.
// The commands in the menu resolve against that selection, so a menu opened
// before it lands is a menu of the wrong node.
bool SelectTreeRow(scada::aui::Tree& tree,
                   AnyExecutor executor,
                   NodeService& node_service,
                   std::span<const QString> path) {
  const QModelIndex index =
      FindTreeRowByPath(tree, executor, node_service, path);
  if (!index.isValid())
    return false;

  tree.scrollTo(index);
  tree.selectionModel()->setCurrentIndex(
      index, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  PumpEvents(20);
  return WaitUntil(
      [&] { return tree.selectionModel()->currentIndex() == index; });
}

// The shared body of the four object-tree captures and the hardware-tree one.
// They differ only in the window they open, the row they select, and which
// menu row they are about — everything else is the same right-click.
void CaptureTreeContextMenu(
    ClientApplication& app,
    AnyExecutor executor,
    const char* filename,
    std::string_view window_type,
    std::span<const QString> node_path,
    const char* highlighted_row,
    const char* open_submenu,
    const std::function<void(const ContextMenuRender&)>& assert_content) {
  MainWindow::SetHideForTesting(false);

  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{std::string{window_type}});
    profile.AddPage(page);
    profile.Save();
  }

  WaitForAwaitable(executor, app.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor, app.node_service()));

  QMainWindow* qmain = ShowMainWindowForMenuCapture(app);
  ASSERT_NE(qmain, nullptr);
  auto* main_window = dynamic_cast<MainWindow*>(qmain);
  ASSERT_NE(main_window, nullptr);

  ExplorerTarget target =
      OpenExplorerTree(app, *main_window, executor, window_type);
  ASSERT_NE(target.tree, nullptr);
  // An empty path means "leave the tree's own default selection alone", which
  // is how a capture reaches the root node: MaterializeExplorerTree re-roots
  // the view at it, so it is not a row anything can click.
  if (!node_path.empty()) {
    ASSERT_TRUE(
        SelectTreeRow(*target.tree, executor, app.node_service(), node_path));
  }

  CapturePublishGuard publish_guard{filename};
  const ContextMenuRender render = OpenContextMenu(
      *main_window, *target.tree, highlighted_row, open_submenu);

  ASSERT_TRUE(render.opened)
      << "the right-click never reached ShowPopupMenu - the tree's "
         "context-menu handler is no longer wired to the controller delegate";
  EXPECT_FALSE(render.rows.isEmpty()) << filename << " rendered an empty menu";
  assert_content(render);

  SaveMenuPixmap(render.pixmap, filename, publish_guard);
}

// The configuration grid («Конфигурация: <группа>»), reached the way the
// manual says to reach it — select a group in the object tree, «Свойства
// элемента» — then right-clicked over a multi-row selection.
//
// It is opened through the menu rather than from a saved page on purpose. The
// saved-page route does open a `TableEditor`, but reaching it the operator's
// way is what guarantees the subject is the group that was selected, and it
// exercises the command the first of these two images is about.
//
// The menu this then captures is a different one from the object tree's: the
// grid passes its own `merge_menu` to `ShowPopupMenu` (the tree passes none),
// so its rows are the table's command set — «Переименовать», «Изменить»,
// «Упорядочить» — merged with the generic one. Driving the real right-click
// is what reaches that set; nothing here had to know the grid has one.
void CaptureGridContextMenu(
    ClientApplication& app,
    AnyExecutor executor,
    const char* filename,
    std::span<const QString> group_path,
    int selected_rows,
    const char* highlighted_row,
    const std::function<void(const ContextMenuRender&)>& assert_content) {
  MainWindow::SetHideForTesting(false);

  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"Struct"});
    profile.AddPage(page);
    profile.Save();
  }

  WaitForAwaitable(executor, app.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(executor, app.node_service()));

  QMainWindow* qmain = ShowMainWindowForMenuCapture(app);
  ASSERT_NE(qmain, nullptr);
  auto* main_window = dynamic_cast<MainWindow*>(qmain);
  ASSERT_NE(main_window, nullptr);

  ExplorerTarget tree_target =
      OpenExplorerTree(app, *main_window, executor, "Struct");
  ASSERT_NE(tree_target.tree, nullptr);
  ASSERT_TRUE(SelectTreeRow(*tree_target.tree, executor, app.node_service(),
                            group_path));

  ASSERT_TRUE(TriggerContextMenuRow(*main_window, *tree_target.tree,
                                    "Element Properties"));

  OpenedView* editor_view = nullptr;
  QTableView* grid = nullptr;
  WaitUntil([&] {
    for (OpenedView* view : main_window->opened_views()) {
      if (view->window_info().name != std::string_view{"TableEditor"})
        continue;
      editor_view = view;
      grid = qobject_cast<QTableView*>(view->view());
      if (!grid && view->view())
        grid = view->view()->findChild<QTableView*>();
      return grid != nullptr;
    }
    return false;
  });
  ASSERT_NE(grid, nullptr) << "«Свойства элемента» opened no configuration "
                              "table";

  main_window->ActivateView(*editor_view);
  PumpEvents(20);

  // The rows arrive over the node service as the model fetches the group's
  // children, so this needs the executor-pumping wait rather than
  // `WaitUntil`, which only drains what Qt already has queued. Several
  // rounds, because each fetched level can schedule the next.
  bool rows_arrived = false;
  for (int attempt = 0; attempt < 10 && !rows_arrived; ++attempt) {
    WaitForPendingNodeLoads(executor, app.node_service());
    PumpEventLoopFor(std::chrono::milliseconds{200});
    rows_arrived = grid->model() && grid->model()->rowCount() >= selected_rows;
  }
  ASSERT_TRUE(rows_arrived)
      << "the configuration table rendered "
      << (grid->model() ? grid->model()->rowCount() : -1)
      << " rows, fewer than the " << selected_rows << " this capture selects";

  // A contiguous multi-row selection, which is what both of these images show
  // and what makes «Удалить» read as "delete these three".
  QItemSelection selection{
      grid->model()->index(0, 0),
      grid->model()->index(selected_rows - 1,
                           grid->model()->columnCount() - 1)};
  grid->selectionModel()->select(
      selection,
      QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
  grid->selectionModel()->setCurrentIndex(grid->model()->index(0, 0),
                                          QItemSelectionModel::NoUpdate);
  PumpEvents(20);

  CapturePublishGuard publish_guard{filename};
  const ContextMenuRender render =
      OpenContextMenu(*main_window, *grid, highlighted_row, nullptr);

  ASSERT_TRUE(render.opened)
      << "the right-click never reached ShowPopupMenu - the grid's "
         "context-menu handler is no longer wired to the controller delegate";
  EXPECT_FALSE(render.rows.isEmpty()) << filename << " rendered an empty menu";
  assert_content(render);

  SaveMenuPixmap(render.pixmap, filename, publish_guard);
}

}  // namespace

// The object tree's context menu on a data item, «Свойства» highlighted
// (dev/data-items.md: "Доступ к просмотру и редактированию конфигурации
// объекта данных возможен через пункты меню `Свойства`...").
//
// The filename says «Параметры», which is what the hand-captured original
// showed: that command was renamed, and the page's prose was already written
// against the new name. The name is kept because `pairs_reviewed` in the
// parity matrix records capture filenames, so renaming one silently
// invalidates every review that named it.
TEST_F(ScreenshotGenerator, CaptureObjectMenuProperties) {
  constexpr const char* kFilename = "menu-parameters.png";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";

  const std::array<QString, 3> path = {QStringLiteral("ЭСТРА-ПС"),
                                       QStringLiteral("ТИ"),
                                       QStringLiteral("Активная мощность")};
  CaptureTreeContextMenu(
      app_, executor_, kFilename, "Struct", path, "Properties", nullptr,
      [](const ContextMenuRender& render) {
        // The manual's image is about the greyed rows as much as the
        // highlighted one: a data item that is neither alarming nor blocked
        // offers neither command, and a menu built without MenuWillShow()
        // would render both enabled.
        EXPECT_FALSE(render.disabled_rows.isEmpty())
            << "no row rendered disabled - the menu's enabled state was not "
               "resolved against the selection";
      });
}

// The object tree's context menu on a data item, «Удалить» highlighted
// (dev/data-items.md, "Удаление объектов": "Пример контекстного меню,
// всплывающего по нажатию ПКМ").
TEST_F(ScreenshotGenerator, CaptureObjectMenuDelete) {
  constexpr const char* kFilename = "menu-delete-object.png";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";

  const std::array<QString, 3> path = {QStringLiteral("ЭСТРА-ПС"),
                                       QStringLiteral("ТИ"),
                                       QStringLiteral("Активная мощность")};
  CaptureTreeContextMenu(app_, executor_, kFilename, "Struct", path, "Delete",
                         nullptr, [](const ContextMenuRender& render) {
                           EXPECT_FALSE(render.disabled_rows.isEmpty())
                               << "no row rendered disabled";
                         });
}

// The object tree's context menu on a group, «Свойства элемента» highlighted
// (dev/displays.md: "Пункт `Свойства элемента` вызывается из контекстного меню
// для выделенной группы объектов по нажатию ПКМ").
TEST_F(ScreenshotGenerator, CaptureObjectMenuElementProperties) {
  constexpr const char* kFilename = "menu-parameters-elements.png";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";

  const std::array<QString, 1> path = {QStringLiteral("ЭСТРА-ПС")};
  CaptureTreeContextMenu(
      app_, executor_, kFilename, "Struct", path, "Element Properties", nullptr,
      [](const ContextMenuRender& render) {
        // A group offers the two properties commands; a data item offers only
        // the singular one. Asserting the plural is here is what says the
        // selection really was a group.
        EXPECT_TRUE(render.rows.contains(RowTitle("Element Properties")))
            << "the group menu has no Element Properties row";
      });
}

// The object tree's context menu on a group with the «Создать» submenu open
// (dev/data-items.md: "Создание одного объекта или серии из нескольких
// объектов внутри выбранной группы осуществляется через меню `Создать`").
TEST_F(ScreenshotGenerator, CaptureObjectMenuCreate) {
  constexpr const char* kFilename = "menu-create-object.png";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";

  const std::array<QString, 1> path = {QStringLiteral("ЭСТРА-ПС")};
  CaptureTreeContextMenu(app_, executor_, kFilename, "Struct", path, "Create",
                         "Create", [](const ContextMenuRender& render) {
                           // The submenu IS the subject of this image, so an
                           // empty one is a failure rather than a thinner
                           // picture.
                           EXPECT_FALSE(render.submenu_rows.isEmpty())
                               << "the Create submenu rendered empty";
                         });
}

// The configuration table's context menu over a multi-row selection,
// «Копировать» highlighted (dev/displays.md: "Из контекстного меню таблицы
// доступны операции *Копирования* и *Вставки* объектов").
TEST_F(ScreenshotGenerator, CaptureGridMenuCopy) {
  constexpr const char* kFilename = "menu-parameters-elements-copy.png";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";

  const std::array<QString, 1> group = {QStringLiteral("ЭНИП-2 + ЭНМВ-1")};
  CaptureGridContextMenu(
      app_, executor_, kFilename, group, 2, "Copy",
      [](const ContextMenuRender& render) {
        // The grid's own command set, which only reaches the menu because the
        // view supplied it as `merge_menu`. A capture that built the menu from
        // the shell's registries alone would render without these and look
        // perfectly plausible.
        EXPECT_TRUE(render.rows.contains(RowTitle("Rename")))
            << "the grid menu lost the table's own commands: "
            << render.rows.join(QLatin1String(" | ")).toStdString();
      });
}

// The configuration table's context menu over a multi-row selection,
// «Удалить» highlighted (development.md: "Удаляет выделенный объект,
// несколько выделенных объектов или устройство из системы").
TEST_F(ScreenshotGenerator, CaptureGridMenuDelete) {
  constexpr const char* kFilename = "ti-delete.png";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";

  const std::array<QString, 1> group = {QStringLiteral("M340")};
  CaptureGridContextMenu(app_, executor_, kFilename, group, 3, "Delete",
                         [](const ContextMenuRender& render) {
                           EXPECT_TRUE(render.rows.contains(RowTitle("Rename")))
                               << "the grid menu lost the table's own commands";
                         });
}
