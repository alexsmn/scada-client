#include "dialog_capture.h"
#include "fixture_builder.h"
#include "graph_capture.h"
#include "screenshot_config.h"
#include "screenshot_modules.h"
#include "screenshot_options.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "widget_capture.h"

#include "address_space/attribute_service_impl.h"
#include "address_space/local_history_service.h"
#include "address_space/local_method_service.h"
#include "address_space/local_monitored_item_service.h"
#include "address_space/local_node_management_service.h"
#include "address_space/local_session_service.h"
#include "address_space/test/scada_test_address_space.h"
#include "address_space/view_service_impl.h"
#include "app/client_application.h"
#include "aui/qt/message_loop_qt.h"
#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"
#include "aui/translation.h"
#include "aui/tree.h"
#include "base/any_executor.h"
#include "base/client_paths.h"
#include "base/no_destructor.h"
#include "base/test/scoped_path_override.h"
#include "controller/window_info.h"
#include "events/qt/event_filter_bar.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "model/data_items_node_ids.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "timed_data/timed_data_service.h"

#include <QAbstractProxyModel>
#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QHeaderView>
#include <QLayout>
#include <QLocale>
#include <QPixmap>
#include <QSettings>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QTableView>
#include <QTemporaryDir>
#include <QToolBar>
#include <QTranslator>
#include <QTreeView>
#include <QVBoxLayout>
#include <boost/asio/io_context.hpp>
#include <gtest/gtest.h>

namespace {

using screenshot_generator::WaitForAwaitable;
using screenshot_generator::WaitForPendingNodeLoads;

// Global config loaded once per test suite.
ScreenshotConfig g_config;

template <class Predicate>
bool WaitUntil(Predicate&& predicate, int timeout_ms = 5000) {
  QElapsedTimer timer;
  timer.start();
  while (!predicate()) {
    if (timer.elapsed() >= timeout_ms)
      return false;
    QApplication::processEvents(QEventLoop::AllEvents, 50);
  }
  return true;
}

aui::Tree* FindTreeWidget(QWidget* widget) {
  if (!widget)
    return nullptr;

  if (auto* tree = dynamic_cast<aui::Tree*>(widget))
    return tree;

  for (auto* child : widget->findChildren<QWidget*>()) {
    if (auto* tree = dynamic_cast<aui::Tree*>(child))
      return tree;
  }

  return nullptr;
}

}  // namespace

class ScreenshotGenerator : public ::testing::Test {
 public:
  static void SetUpTestSuite() {
    // Hermetic settings: the client reads default-constructed QSettings
    // (registry on Windows, plists on macOS), so on a used dev box the
    // captures would inherit real state — e.g. the last-used server
    // address in the login dialog. Redirect the default format into a
    // scratch ini tree so every run renders from a factory-fresh profile
    // regardless of the machine.
    static base::NoDestructor<QTemporaryDir> settings_dir;
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                       settings_dir->path());
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope,
                       settings_dir->path());
    QSettings::setDefaultFormat(QSettings::IniFormat);

    InitScreenshotOptions();
    g_config.Load(GetDataFilePath());
  }

  ScreenshotGenerator();
  ~ScreenshotGenerator();

 protected:
  boost::asio::io_context io_context_;
  // QApplication must exist before MessageLoopQt — the latter wires a
  // QTimer in its ctor — so app_env_ is declared first.
  AppEnvironment app_env_;
  AnyExecutor executor_ = MakeAnyExecutor(std::make_shared<MessageLoopQt>());

  // Russian translator, installed in the constructor body. Must outlive
  // the QApplication inside `app_env_`, hence declared right after it.
  QTranslator translator_;

  // SCADA back-end. The address space starts pre-populated with the
  // standard OPC UA + SCADA folder/type tree (code-defined by
  // ScadaTestAddressSpace, no nodeset XML); ns=1 instance nodes from
  // `screenshot_data.json` get added on top in the test fixture's
  // constructor body.
  scada_test::ScadaTestAddressSpace address_space_;
  SyncAttributeServiceImpl sync_attribute_service_{
      AttributeServiceImplContext{address_space_}};
  AttributeServiceImpl attribute_service_{sync_attribute_service_};
  SyncViewServiceImpl sync_view_service_{
      ViewServiceImplContext{address_space_}};
  ViewServiceImpl view_service_{sync_view_service_};
  scada::LocalHistoryService history_service_;
  scada::LocalMonitoredItemService monitored_item_service_;
  scada::LocalMethodService method_service_;
  scada::LocalNodeManagementService node_management_service_;
  scada::LocalSessionService session_service_;

  scada::services services_{
      .attribute_service = &attribute_service_,
      .monitored_item_service = &monitored_item_service_,
      .method_service = &method_service_,
      .history_service = &history_service_,
      .view_service = &view_service_,
      .node_management_service = &node_management_service_,
      .session_service = &session_service_,
  };

  base::ScopedPathOverride private_dir_override_{client::DIR_PRIVATE};

  ClientApplication app_{ClientApplicationContext{
      .io_context_ = io_context_,
      .executor_ = executor_,
      .login_handler_ = [this](DataServicesContext&&)
          -> Awaitable<std::optional<DataServices>> {
        co_return std::optional{DataServices::FromUnownedServices(services_)};
      },
      // Intentionally no node_service/timed_data_service/tree-factory
      // overrides: we let ClientApplication build the production
      // `v1::NodeServiceImpl`, `TimedDataServiceImpl`, and default
      // `NodeServiceTreeImpl` on top of the Local* services. The node
      // graph is populated on demand by the address-space fetcher over
      // `LocalViewService::Browse` + `LocalAttributeService::Read`,
      // which is how current values, display names, and tree structure
      // all flow through the real code paths the client runs in prod.
      .module_configurator_ = MakeScreenshotModules()}};
};

ScreenshotGenerator::ScreenshotGenerator() {
  // Install the Russian translator directly. We intentionally bypass
  // InstalledTranslation here: it reads the locale from a QSettings
  // instance that lacks an organization/app name (AppEnvironment builds
  // a bare QApplication), so the value round-trips through an
  // unreliable backing store and the Russian .qm ends up not loading
  // at all.
  //
  // The install must happen per-fixture: each TEST_F spins up a fresh
  // QApplication via AppEnvironment, and QTranslator registrations
  // don't survive across QApplication instances.
  QLocale::setDefault(QLocale{QLocale::Russian, QLocale::Russia});
  const auto translation_dir =
      QApplication::applicationDirPath() + "/translations";
  if (translator_.load("client_ru", translation_dir))
    QApplication::installTranslator(&translator_);

  // Match the default client style. Set directly rather than through
  // InstalledStyle for the same QSettings reason: it also writes back
  // the live style's objectName on destruction, which would let the
  // second TEST_F pick up whatever QStyleFactory returned instead of
  // "Fusion".
  QApplication::setStyle("Fusion");

  // Optionally render under a UX design-token theme so captures validate the
  // reshell against real Qt widgets (--theme=dark|light|hc). Applied over the
  // Fusion base exactly as app/qt/main.cpp does when the experimental UX is on.
  if (const std::string& theme_name = GetScreenshotOptions().theme;
      !theme_name.empty()) {
    const scada::aui::Theme theme = scada::aui::ThemeFromString(
        QString::fromStdString(theme_name), scada::aui::Theme::kDark);
    scada::aui::ApplyTheme(theme);
    scada::aui::SetSeverityTheme(theme == scada::aui::Theme::kLight
                                     ? scada::aui::SeverityTheme::kLight
                                 : theme == scada::aui::Theme::kHighContrast
                                     ? scada::aui::SeverityTheme::kHighContrast
                                     : scada::aui::SeverityTheme::kDark);
  }

  // Render offscreen. `widget->grab()` renders the Qt widget tree to a
  // QPixmap without needing the window to be on-screen — so this
  // suite works headless (incl. CI without a desktop session).
  // Trade-off: we don't get the native OS title bar / frame; we tried
  // PrintWindow(PW_RENDERFULLCONTENT) for that and it didn't reliably
  // capture child-widget content (see commit history).
  MainWindow::SetHideForTesting();

  // Populate the address space with ns=1 instance nodes; everything
  // else (standard OPC UA / SCADA tree) was already built in code by
  // ScadaTestAddressSpace. The real `v1::NodeServiceImpl`
  // inside ClientApplication browses and reads them through
  // ViewServiceImpl + AttributeServiceImpl on demand.
  PopulateFixtureNodes(address_space_, g_config.json);
  history_service_.LoadFromJson(g_config.json);
}

ScreenshotGenerator::~ScreenshotGenerator() {
  WaitForAwaitable(executor_, app_.Quit());
}

#if !defined(UI_WT)

TEST_F(ScreenshotGenerator, CaptureAllWindows) {
  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);

  {
    Profile profile;
    // Keep screenshot windows stable: the fixture currently seeds only
    // historical events, so the normal auto-hide policy would close the
    // Event pane during startup before CaptureAllWindows inspects it.
    profile.event_auto_show = false;
    profile.event_auto_hide = false;
    profile.AddPage(MakeScreenshotPage(g_config.screenshots, g_config.json));
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  // Let async data loads and model updates complete.
  screenshot_generator::PumpEventLoopFor(std::chrono::seconds(1));

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_EQ(main_windows.size(), 1u);
  const MainWindow& main_window = main_windows.front();

  int captured = 0;
  for (const auto& spec : g_config.screenshots) {
    OpenedView* view = nullptr;
    for (OpenedView* v : main_window.opened_views()) {
      if (v->window_info().name == spec.window_type) {
        view = v;
        break;
      }
    }

    if (!view) {
      ADD_FAILURE() << "Window type not found: " << spec.window_type;
      continue;
    }

    QWidget* widget = view->view();
    if (!widget) {
      ADD_FAILURE() << "No QWidget for: " << spec.window_type;
      continue;
    }

    // A grid-backed window that renders fewer rows than the fixture defines
    // is a data-path regression (empty users/transmission tables have
    // shipped as "successful" captures before) — fail loudly instead of
    // silently saving a bare frame.
    if (spec.min_rows > 0) {
      int max_rows = 0;
      QList<QTableView*> tables = widget->findChildren<QTableView*>();
      if (auto* table = qobject_cast<QTableView*>(widget))
        tables.prepend(table);
      for (const QTableView* table : tables) {
        if (table->model())
          max_rows = std::max(max_rows, table->model()->rowCount());
      }
      EXPECT_GE(max_rows, spec.min_rows)
          << spec.filename << ": the " << spec.window_type
          << " grid rendered fewer rows than the fixture populates - the "
             "capture would be empty or partial";
    }

    if (spec.window_type == "Graph") {
      // Render graph standalone — hidden main windows don't lay out
      // QSplitter children, so we build a fresh graph widget.
      SaveGraphScreenshot(spec, app_.timed_data_service(), g_config.json);
    } else {
      SaveScreenshot(widget, spec);
    }
    ++captured;
  }

  std::cout << "Captured " << captured << "/" << g_config.screenshots.size()
            << " screenshots to " << output_dir.string() << std::endl;
}

TEST_F(ScreenshotGenerator, CaptureMainWindow) {
  if (!ShouldCaptureScreenshot("client-window.png"))
    GTEST_SKIP() << "client-window.png not requested";

  MainWindow::SetHideForTesting(false);

  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);
  const auto output_image = output_dir / "client-window.png";

  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"EventJournal"});
    page.AddWindow(WindowDefinition{"Summ"});
    page.AddWindow(WindowDefinition{"Struct"});
    profile.AddPage(page);
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_EQ(main_windows.size(), 1u);
  auto& main_window = main_windows.front();

  auto* qmain = dynamic_cast<QWidget*>(&main_window);
  ASSERT_NE(qmain, nullptr);
  qmain->resize(1920, 1080);
  qmain->ensurePolished();
  qmain->show();
  if (auto* central = qmain->layout())
    central->activate();
  if (auto* central_widget = qmain->findChild<QWidget*>())
    if (auto* layout = central_widget->layout())
      layout->activate();
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  // Verify the toolbar icon wiring against the live QWidget tree directly,
  // not the rendered pixels: the style must be icon-only, and the icon-bearing
  // command actions must have resolved their icons through LoadPixmap, which
  // returned empty pixmaps on macOS/Linux before the res/client.qrc fix. Not
  // every command carries an icon (many are image_id == 0, text-only), so we
  // assert that the pipeline produced icons, not that every action has one;
  // exhaustive per-id coverage lives in ClientUtilsQtTest.
  // Target the command toolbar specifically: the opt-in reshell adds other
  // toolbars (activity rail, context bar) that carry no command icons.
  auto* toolbar = qmain->findChild<QToolBar*>("CommandToolbar");
  ASSERT_NE(toolbar, nullptr);
  EXPECT_EQ(toolbar->toolButtonStyle(), Qt::ToolButtonIconOnly);
  int actions_with_icons = 0;
  for (QAction* action : toolbar->actions()) {
    if (!action->isSeparator() && !action->icon().isNull())
      ++actions_with_icons;
  }
  EXPECT_GT(actions_with_icons, 0)
      << "no toolbar action resolved an icon (LoadPixmap/qrc regression)";

  aui::Tree* tree = nullptr;
  QDockWidget* tree_dock = nullptr;
  for (OpenedView* view : main_window.opened_views()) {
    if (view->window_info().name != "Struct")
      continue;

    tree = FindTreeWidget(view->view());
    tree_dock = qobject_cast<QDockWidget*>(view->view()->parentWidget());
    break;
  }
  ASSERT_NE(tree, nullptr);

  if (tree_dock) {
    tree_dock->show();
    tree_dock->raise();
    tree_dock->setMinimumWidth(420);
    main_window.resizeDocks({tree_dock}, {480}, Qt::Horizontal);
  }
  tree->show();

  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  QModelIndex root_index = tree->model()->index(0, 0, tree->rootIndex());
  if (!root_index.isValid())
    root_index = tree->model()->index(0, 0);
  ASSERT_TRUE(root_index.isValid());

  if (tree->model()->canFetchMore(root_index))
    tree->model()->fetchMore(root_index);
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  // For the screenshot we want the object rows, not the synthetic tree root.
  // Making the fetched root the view root sidesteps the "expanded root with
  // empty child viewport" state that Qt sometimes gets into here.
  tree->setRootIndex(root_index);
  tree->setRootIsDecorated(true);
  tree->expand(tree->rootIndex());
  ASSERT_TRUE(WaitUntil([&] { return tree->isExpanded(tree->rootIndex()); }));
  tree->expandRecursively(tree->rootIndex(), 3);
  tree->resizeColumnToContents(0);
  tree->doItemsLayout();
  tree->viewport()->update();

  auto* proxy_model = qobject_cast<QAbstractProxyModel*>(tree->model());
  QModelIndex materialized_source_root;
  const bool first_child_visible = WaitUntil(
      [&] {
        const auto& visible_root = tree->rootIndex();
        if (tree->model()->canFetchMore(visible_root))
          tree->model()->fetchMore(visible_root);

        auto first_child = tree->model()->index(0, 0, visible_root);
        if (!first_child.isValid() && proxy_model) {
          materialized_source_root = proxy_model->mapToSource(visible_root);
          if (materialized_source_root.isValid()) {
            const auto source_first_child = proxy_model->sourceModel()->index(
                0, 0, materialized_source_root);
            if (source_first_child.isValid())
              first_child = proxy_model->mapFromSource(source_first_child);
          }

          if (!first_child.isValid() && materialized_source_root.isValid() &&
              proxy_model->sourceModel()->rowCount(materialized_source_root) >
                  0) {
            tree->model()->sort(0);
            tree->collapse(visible_root);
            QApplication::processEvents();
            tree->expand(visible_root);
            first_child = tree->model()->index(0, 0, visible_root);

            if (!first_child.isValid()) {
              const auto source_first_child = proxy_model->sourceModel()->index(
                  0, 0, materialized_source_root);
              if (source_first_child.isValid())
                first_child = proxy_model->mapFromSource(source_first_child);
            }
          }
        }

        if (!first_child.isValid())
          return false;

        tree->scrollTo(first_child);
        tree->doItemsLayout();
        tree->viewport()->update();
        return !tree->visualRect(first_child).isEmpty();
      },
      2000);
  int source_child_count = -1;
  if (proxy_model && materialized_source_root.isValid()) {
    source_child_count =
        proxy_model->sourceModel()->rowCount(materialized_source_root);
  }

  if (!first_child_visible) {
    const auto& visible_root = tree->rootIndex();
    const auto proxy_child_count = tree->model()->rowCount(visible_root);
    const auto first_child = tree->model()->index(0, 0, visible_root);
    const auto first_child_rect =
        first_child.isValid() ? tree->visualRect(first_child) : QRect{};
    const auto root_rect = tree->visualRect(visible_root);
    const auto viewport_size = tree->viewport()->size();

    ADD_FAILURE() << "Struct tree did not materialize visible rows before "
                  << "capture"
                  << " | proxy_child_count=" << proxy_child_count
                  << " | source_child_count=" << source_child_count
                  << " | viewport=" << viewport_size.width() << "x"
                  << viewport_size.height() << " | root_rect=" << root_rect.x()
                  << "," << root_rect.y() << " " << root_rect.width() << "x"
                  << root_rect.height()
                  << " | first_child_valid=" << first_child.isValid()
                  << " | first_child_rect=" << first_child_rect.x() << ","
                  << first_child_rect.y() << " " << first_child_rect.width()
                  << "x" << first_child_rect.height()
                  << " | tree_visible=" << tree->isVisible()
                  << " | viewport_visible=" << tree->viewport()->isVisible()
                  << " | dock_visible="
                  << (tree_dock ? tree_dock->isVisible() : true);
  }
  ASSERT_TRUE(first_child_visible);
  ASSERT_TRUE(WaitUntil([&] {
    const auto loading_suffix =
        QString::fromStdU16String(u"[" + Translate("Loading") + u"]");
    for (int row = 0; row < tree->model()->rowCount(tree->rootIndex()); ++row) {
      const auto index = tree->model()->index(row, 0, tree->rootIndex());
      if (index.data(Qt::DisplayRole).toString().contains(loading_suffix))
        return false;
    }
    return true;
  }));
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  QPixmap pixmap = qmain->grab();
  pixmap.save(QString::fromStdString(output_image.string()));

  MainWindow::SetHideForTesting(true);
}

// Regression test for a stack overflow that fires during `app_.Start()`
// when a page containing a Struct (tree) window is loaded on top of the
// in-memory address space.
//
// The fixture wires the real `v1::NodeServiceImpl` and
// `NodeServiceTreeImpl` over synchronous Local* services that complete
// fetch callbacks in the caller's stack frame. During boot the tree
// model opens the root; each `ConfigurationTreeNode` ctor calls
// `node_.Fetch(NodeOnly)`; the fetch completes synchronously, fires
// `OnNodeChildrenChanged`, which re-enters `UpdateChildTreeNodes`, which
// creates more `ConfigurationTreeNode` children, each calling `Fetch()`
// again — and so on until the stack runs out. In production (gRPC)
// fetches return async over a socket, so the chain stays shallow and
// the bug never surfaces.
//
// This test is the smallest repro: only a Struct window on the page,
// no screenshots, no dialogs. The bug fix should let `app_.Start()`
// return normally and leave one main window open.
TEST_F(ScreenshotGenerator, BootWithStructPageDoesNotOverflowStack) {
  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"Struct"});
    profile.AddPage(page);
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());

  // Let the initial address-space fetch cascade complete.
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  // If the fetch cascade overflowed the stack the process would have
  // aborted before this line — reaching here means the recursion is
  // bounded.
  EXPECT_EQ(app_.main_window_manager().main_windows().size(), 1u);
}

// Verifies the event journal's Area filter populates at runtime: the same
// enumeration the filter bar drives (`BrowseEventAreas`), run against the real
// `v1::NodeServiceImpl` over the fixture address space, returns the operator's
// top-level area groupings and drops leaf data items.
TEST_F(ScreenshotGenerator, EventFilterBarEnumeratesAreas) {
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  NodeService& node_service = app_.node_service();
  std::vector<EventAreaEntry> areas =
      WaitForAwaitable(executor_, BrowseEventAreas(node_service));

  // The Area dropdown is populated from this list — it must not be empty.
  ASSERT_FALSE(areas.empty());

  // Every enumerated area is a named object grouping, never a leaf data item —
  // that is the level the operator filters the journal by.
  for (const EventAreaEntry& area : areas) {
    EXPECT_FALSE(area.name.empty());
    NodeRef node = node_service.GetNode(area.node_id);
    EXPECT_FALSE(IsInstanceOf(node, data_items::id::DataItemType))
        << "an area must not be a leaf data item";
  }

  // The DataItems root also holds loose top-level data items; the enumeration
  // partitions its Organizes children exactly into areas + excluded leaves.
  std::vector<NodeRef> children = node_service.GetTargets(
      data_items::id::DataItems, scada::id::Organizes, /*forward=*/true);
  size_t leaf_count = 0;
  for (NodeRef& child : children) {
    if (IsInstanceOf(child, data_items::id::DataItemType))
      ++leaf_count;
  }
  EXPECT_EQ(areas.size() + leaf_count, children.size());
}

TEST_F(ScreenshotGenerator, CaptureDialogs) {
  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);

  // Start the app so the real TimedDataServiceImpl is wired up; the
  // WriteDialog family reads current values, formula titles, and
  // engineering units through it.
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  Profile profile;
  DialogEnvironment env{
      .executor = executor_,
      .node_service = &app_.node_service(),
      .timed_data_service = &app_.timed_data_service(),
      .profile = &profile,
      .dialog_analog_node_id = g_config.dialog_analog_node_id};

  int captured = 0;
  for (const auto& spec : g_config.dialogs) {
    if (CaptureDialog(spec, env))
      ++captured;
  }

  std::cout << "Captured " << captured << "/" << g_config.dialogs.size()
            << " dialogs to " << output_dir.string() << std::endl;
}

#endif
