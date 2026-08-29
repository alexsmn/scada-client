#include "administration_capture.h"
#include "bulk_create_capture.h"
#include "command_field_capture.h"
#include "debugger_capture.h"
#include "device_diagnostics_capture.h"
#include "device_metrics_capture.h"
#include "dialog_capture.h"
#include "display_capture.h"
#include "fixture_builder.h"
#include "frame_decode_capture.h"
#include "graph_capture.h"
#include "inspector_capture.h"
#include "null_task_manager.h"
#include "screenshot_config.h"
#include "screenshot_fixture.h"
#include "screenshot_modules.h"
#include "screenshot_options.h"
#include "screenshot_output.h"
#include "screenshot_wait.h"
#include "severity_tiles_capture.h"
#include "transmission_rule_capture.h"
#include "user_access_capture.h"
#include "view_capture.h"
#include "publish_guard.h"
#include "widget_capture.h"

#include "app/client_application.h"
#include "aui/qt/theme_qt.h"
#include "aui/severity_colors.h"
#include "aui/translation.h"
#include "aui/tree.h"
#include "base/utf_convert.h"
#include "common/format.h"
#include "controller/window_info.h"
#include "main_window/main_menu/main_menu_model.h"
#include "main_window/main_window.h"
#include "main_window/main_window_manager.h"
#include "main_window/opened_view/opened_view.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "model/security_node_ids.h"
#include "modules/limits/limit_model.h"
#include "modules/write/write_model.h"
#include "node_service/node_awaitable.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "settings/qt/settings_panel.h"
#include "timed_data/timed_data_service.h"

#include <QAbstractButton>
#include <QAbstractProxyModel>
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDockWidget>
#include <QElapsedTimer>
#include <QHeaderView>
#include <QItemSelectionModel>
#include <QLabel>
#include <QLayout>
#include <QLibraryInfo>
#include <QListWidget>
#include <QLocale>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QPixmap>
#include <QStackedWidget>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QString>
#include <QTableView>
#include <QToolBar>
#include <QToolButton>
#include <QTreeView>
#include <QVBoxLayout>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <set>
#include <span>
#include <string>
#include <string_view>

namespace {

using scada::screenshot_generator::CaptureViewSpec;
using scada::screenshot_generator::FindTreeWidget;
using scada::screenshot_generator::FixtureConfig;
using scada::screenshot_generator::ScreenshotGenerator;
using scada::screenshot_generator::SeedFavourites;
using scada::screenshot_generator::SeedLocalEvents;
using scada::screenshot_generator::ShowMainWindowForMenuCapture;
using scada::screenshot_generator::ViewCaptureContext;
using scada::screenshot_generator::WaitForAwaitable;
using scada::screenshot_generator::WaitForPendingNodeLoads;
using scada::screenshot_generator::WaitUntil;

}  // namespace

namespace {

// Everything a standalone capture can ask for. The dispatch table below holds
// one signature instead of seventeen, so this bundle carries the union of what
// the capture functions take; most entries use two or three of its members.
struct StandaloneCaptureContext {
  const ScreenshotSpec& spec;
  MainWindow& main_window;
  NodeService& node_service;
  TimedDataService& timed_data_service;
  scada::AttributeService& authenticated_attribute_service;
  const boost::json::value& json;
  AnyExecutor executor;
};

// One standalone capture: the `capture` key a fixture spec selects it by, and
// the function that renders it. A null `save` means the spec is rendered by
// another TEST_F, so the sweep skips it without counting it as captured.
struct StandaloneCapture {
  std::string_view key;
  void (*save)(const StandaloneCaptureContext&);
};

// The `capture` keys, in one place. This was seventeen hand-written `else if`
// arms until 2026-08-16; the shape cost a `++captured` once already (d8dd095c9
// inserted a branch between a comment and its `if`, so the run's tally
// under-reported by one from 2026-07-27 until it was found). A table cannot
// lose that line, because no entry carries it.
constexpr StandaloneCapture kStandaloneCaptures[] = {
    // The series inspector is standalone chrome, not a window on the page —
    // build it from the graph fixture instead of looking up an opened view.
    {"series-inspector",
     +[](const StandaloneCaptureContext& c) {
       SaveSeriesInspectorScreenshot(c.spec, c.node_service,
                                     c.timed_data_service, c.json);
     }},
    // The device-diagnostics panel is standalone reshell chrome (the right
    // region of config-workbench.html), built from a fixture device rather
    // than an opened page view.
    {"device-diagnostics",
     +[](const StandaloneCaptureContext& c) {
       SaveDeviceDiagnosticsScreenshot(
           c.spec, c.node_service, c.timed_data_service, c.json, c.executor);
     }},
    // The device Metrics sheet is a CusTable whose cells DeviceMetricsModule
    // derives from the device's type-definition data variables, so it can only
    // be built once the node service has resolved the device — after the
    // profile page was assembled. It opens its own view.
    {"device-metrics",
     +[](const StandaloneCaptureContext& c) {
       SaveDeviceMetricsScreenshot(c.spec, c.main_window, c.node_service,
                                   c.timed_data_service, c.executor);
     }},
    // The Administration explorer is the left region of users-admin.html. It
    // derives its rows from the shell's command resolution, which the headless
    // generator has no shell for, so the capture supplies the section set.
    {"administration",
     +[](const StandaloneCaptureContext& c) {
       SaveAdministrationScreenshot(c.spec);
     }},
    // The users-admin RBAC inspector is standalone reshell chrome (the right
    // region of users-admin.html), built from a fixture user.
    {"user-access",
     +[](const StandaloneCaptureContext& c) {
       SaveUserAccessScreenshot(c.spec, c.node_service,
                                c.authenticated_attribute_service, c.executor);
     }},
    // The Roles view needs the same administrator identity: its grid is built
    // from the server's published role -> permission map, which an anonymous
    // session may not read. Opened as an ordinary view it rendered
    // "Roles · no data" and saved an empty grid.
    {"roles",
     +[](const StandaloneCaptureContext& c) {
       SaveRolesScreenshot(c.spec, c.node_service,
                           c.authenticated_attribute_service, c.executor);
     }},
    // The users-admin grid joins its Roles column from the same RoleSet read,
    // so it needs the same identity — through the view path every account's
    // Roles cell read "Нет данных". This is the reshell panel, not the legacy
    // `users.png` Users window, which is a `type` spec and keeps going through
    // the profile page. `MakeUsersGridPanel` returns nothing outside the
    // reshell, so an unthemed run says so rather than saving the legacy view
    // under this filename.
    {"users-grid",
     +[](const StandaloneCaptureContext& c) {
       SaveUsersGridScreenshot(c.spec, c.node_service,
                               c.authenticated_attribute_service, c.executor);
     }},
    // The transmission-rule inspector is standalone reshell chrome (the right
    // region of transmission-rules.html), built from a fixture transmission
    // item.
    {"transmission-rule",
     +[](const StandaloneCaptureContext& c) {
       SaveTransmissionRuleScreenshot(c.spec, c.node_service, c.executor);
     }},
    // The bulk-create preview is standalone reshell chrome (the center of
    // bulk-create.html), built from a demo pattern with no node service.
    {"bulk-create",
     +[](const StandaloneCaptureContext& c) {
       SaveBulkCreateScreenshot(c.spec);
     }},
    // The protocol debugger is a --debug-gated window, not a registered view,
    // so the ordinary view sweep cannot reach it; it is built over a fixture
    // request trace.
    {"debugger",
     +[](const StandaloneCaptureContext& c) {
       SaveDebuggerScreenshot(c.spec);
     }},
    // The device-log filter bar is a strip of stock widgets, built on its own
    // rather than reached through WatchView.
    {"watch-filter-bar",
     +[](const StandaloneCaptureContext& c) {
       SaveWatchFilterBarScreenshot(c.spec);
     }},
    // The frame-decode pane is the device log's inspector; it is built over a
    // fixture APDU because reaching it through WatchView would mean assembling
    // a full ControllerContext.
    {"frame-decode",
     +[](const StandaloneCaptureContext& c) {
       SaveFrameDecodeScreenshot(c.spec);
     }},
    // The KPI severity tiles are standalone reshell chrome (the context bar's
    // alarm summary), built from seeded counts with no node service.
    {"severity-tiles",
     +[](const StandaloneCaptureContext& c) {
       SaveSeverityTilesScreenshot(c.spec);
     }},
    // The command/search field is standalone reshell chrome (the context bar's
    // palette entry point), built with the same prompt and shortcut the window
    // gives it.
    {"command-field",
     +[](const StandaloneCaptureContext& c) {
       SaveCommandFieldScreenshot(c.spec);
     }},
    // The Inspector is standalone reshell chrome (the right-hand selection
    // panel), filled with a representative expression-row selection.
    {"inspector",
     +[](const StandaloneCaptureContext& c) {
       SaveInspectorScreenshot(c.spec);
     }},
    // The Inspector's event (alarm) card for a journal-row selection.
    {"inspector-event",
     +[](const StandaloneCaptureContext& c) {
       SaveInspectorEventScreenshot(c.spec);
     }},
    // The substation display needs a DisplayFrame of its own and is rendered
    // by the CaptureDisplay TEST_F; nothing for this sweep to grab. Null
    // rather than absent, so an unknown key is still an error.
    {"display", nullptr},
};

// Resolves a spec's `capture` key. Null means the key is not one this
// generator knows, which is a fixture error rather than a skip.
const StandaloneCapture* FindStandaloneCapture(std::string_view key) {
  for (const StandaloneCapture& entry : kStandaloneCaptures) {
    if (entry.key == key)
      return &entry;
  }
  return nullptr;
}

}  // namespace

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
    profile.AddPage(
        MakeScreenshotPage(FixtureConfig().screenshots, FixtureConfig().json));
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());

  // After Start, because the favourites store is built during post-login. The
  // pane's model subscribes to additions, so rows added now still reach it.
  SeedFavourites(FixtureConfig().json, app_.favourites());
  SeedLocalEvents(FixtureConfig().json, app_.local_events());

  // Wait for the data itself rather than pumping for a fixed second and
  // hoping: with many windows open that second was split too many ways, and a
  // view could be grabbed before its trends arrived.
  ASSERT_TRUE(scada::screenshot_generator::WaitForPendingData(
      app_.node_service(), app_.timed_data_service()));

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_EQ(main_windows.size(), 1u);
  // Non-const: capturing a sidebar pane means selecting its activity-rail mode
  // first, the same way an operator would.
  MainWindow& main_window = const_cast<MainWindow&>(main_windows.front());

  int captured = 0;
  // Each non-standalone spec adds its own window to the fixture page (see
  // MakeScreenshotPage), so several specs can share a window_type — e.g. the
  // config-workbench "NewProps" specs (config-parameters, config-address-map on
  // TS.702; config-limits on TS.114). Consume the matching opened views in
  // page order so the Nth spec of a type gets the Nth view, instead of every
  // spec re-grabbing the first one (which pointed config-limits at TS.702's
  // form, whose Limits subtab does not exist).
  std::set<const OpenedView*> used_views;
  for (const auto& spec : FixtureConfig().screenshots) {
    // Standalone captures dispatch on `capture` — the screenshots-side twin of
    // `DialogSpec::kind`. Each builds its own fixture instead of grabbing an
    // opened view, and `MakeScreenshotPage` keeps every one of them off the
    // profile page.
    if (!spec.capture.empty()) {
      const StandaloneCapture* entry = FindStandaloneCapture(spec.capture);
      if (!entry) {
        ADD_FAILURE() << spec.filename << ": unknown capture: " << spec.capture;
        continue;
      }
      // A null handler is a spec another TEST_F renders; skip it without
      // counting it, which is what the old `display` arm's bare `continue`
      // did.
      if (!entry->save)
        continue;
      entry->save(StandaloneCaptureContext{
          .spec = spec,
          .main_window = main_window,
          .node_service = app_.node_service(),
          .timed_data_service = app_.timed_data_service(),
          .authenticated_attribute_service = authenticated_attribute_service_,
          .json = FixtureConfig().json,
          .executor = executor_,
      });
      ++captured;
      continue;
    }

    if (!CaptureViewSpec(spec,
                         ViewCaptureContext{
                             .main_window = main_window,
                             .node_service = app_.node_service(),
                             .timed_data_service = app_.timed_data_service(),
                             .json = FixtureConfig().json,
                         },
                         used_views)) {
      continue;
    }
    ++captured;
  }

  std::cout << "Captured " << captured << "/"
            << FixtureConfig().screenshots.size() << " screenshots to "
            << output_dir.string() << std::endl;
}

TEST_F(ScreenshotGenerator, CaptureDisplay) {
  // The reshelled substation display renders standalone from a VDS fixture — it
  // isn't part of the profile page, so it can't be picked up by
  // CaptureAllWindows' view-matching loop. Cross-platform: the VDS renderer
  // paints without the Windows-only Modus/Vidicon ActiveX host.
  const ScreenshotSpec* display_spec = nullptr;
  for (const auto& spec : FixtureConfig().screenshots) {
    if (spec.capture == "display") {
      display_spec = &spec;
      break;
    }
  }
  if (!display_spec || !ShouldCaptureScreenshot(display_spec->filename))
    GTEST_SKIP() << "Display capture not requested";

  // The bay strips need the live services, so the app runs for this capture
  // exactly as it does for the view captures.
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  std::filesystem::create_directories(GetOutputDir());
  SaveDisplayScreenshot(*display_spec, FixtureConfig().json,
                        app_.timed_data_service(), app_.node_event_provider(),
                        app_.node_service());
}

namespace {

// Walks the Explorer tree from its visible root down `path` (one display name
// per level), fetching and expanding each level on the way, and returns the
// index of the last name. The rows are lazily loaded, so a level has to be
// fetched and settled before the next name can be looked for.
//
// An invalid index means a step was not found, and the failure names the level
// and the rows it did see: the caller uses this to put a selection on screen,
// and a silently missed row would publish the state the selection exists to
// replace.
QModelIndex FindTreeRowByPath(scada::aui::Tree& tree,
                              NodeService& node_service,
                              std::span<const QString> path) {
  QModelIndex parent = tree.rootIndex();
  for (size_t level = 0; level < path.size(); ++level) {
    const QString& name = path[level];
    QModelIndex found;
    const bool level_has_name = WaitUntil([&] {
      if (tree.model()->canFetchMore(parent))
        tree.model()->fetchMore(parent);
      for (int row = 0; row < tree.model()->rowCount(parent); ++row) {
        const QModelIndex index = tree.model()->index(row, 0, parent);
        if (index.data(Qt::DisplayRole).toString() == name) {
          found = index;
          return true;
        }
      }
      return false;
    });

    if (!level_has_name) {
      QStringList seen;
      for (int row = 0; row < tree.model()->rowCount(parent); ++row) {
        seen << tree.model()
                    ->index(row, 0, parent)
                    .data(Qt::DisplayRole)
                    .toString();
      }
      ADD_FAILURE() << "Explorer row not found: " << name.toStdString()
                    << " | siblings="
                    << seen.join(QLatin1String(", ")).toStdString();
      return {};
    }

    // Expanding fetches the next level; the leaf is left as the operator would
    // leave it, so selecting a signal does not open a branch under it.
    if (level + 1 < path.size()) {
      tree.expand(found);
      if (!WaitForPendingNodeLoads(node_service))
        return {};
    }
    parent = found;
  }
  return parent;
}

// Puts a selection on screen for the workbench capture, because the Inspector
// is a panel that shows the current one (docs/client/ux/shell.md §2.5) and an
// unselected shell renders it as its "select an item" placeholder — so the
// published hero documented the panel with the one state that says nothing
// about what it holds.
//
// The selection is made the way the shell's own flow makes one ("Selection is
// global": Explorer selection → Inspector): activate the Explorer, then select
// the row. Everything downstream is the live path —
// ConfigurationTreeView::UpdateSelection → SelectionModel →
// MainWindow::OnSelectionChanged → InspectorPanel::ShowSelection — so the
// capture exercises that wiring rather than filling the panel by hand, which is
// what the standalone inspector-panel.png does.
void SelectSignalForInspector(MainWindow& main_window,
                              OpenedView& explorer_view,
                              scada::aui::Tree& tree,
                              NodeService& node_service,
                              QWidget& window) {
  // Активная мощность is an analog item under the fixture's telemetry folder,
  // and it is the signal two of the journal rows are about — so the window
  // shows one story rather than two.
  const std::array<QString, 3> inspected_path = {
      QStringLiteral("ЭСТРА-ПС"), QStringLiteral("ТИ"),
      QStringLiteral("Активная мощность")};
  const QModelIndex inspected =
      FindTreeRowByPath(tree, node_service, inspected_path);
  ASSERT_TRUE(inspected.isValid());

  main_window.ActivateView(explorer_view);
  tree.scrollTo(inspected);
  tree.selectionModel()->setCurrentIndex(
      inspected,
      QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

  // The card fills in two steps — ShowSelection switches to the element page
  // synchronously, and the readout arrives with the value — so wait for the
  // second one. A "—" readout is the failure this guards: it means the panel is
  // on screen holding nothing, which is the state the selection was made to
  // avoid.
  auto* stack = window.findChild<QStackedWidget*>("inspectorStack");
  ASSERT_NE(stack, nullptr);
  auto* value = window.findChild<QLabel*>("inspectorValue");
  ASSERT_NE(value, nullptr);
  const bool filled = WaitUntil([&] {
    return stack->currentIndex() == 1 && !value->text().isEmpty() &&
           value->text() != QStringLiteral("—");
  });
  EXPECT_TRUE(filled) << "Inspector did not fill for the Explorer selection"
                      << " | page=" << stack->currentIndex()
                      << " | value=" << value->text().toStdString();

  // The Measurements limits block arrives on its own schedule: the bands are
  // property children, so the panel asks the shell to fetch them
  // (InspectorPanelContext::load_limits) and redraws when they land. Asserting
  // it is the point — a card that renders a live value with no limits beside it
  // is what principles.md §2 exists to prevent, and it is exactly what this
  // capture published until the fetch was wired.
  auto* limits = window.findChild<QWidget*>("inspectorLimits");
  ASSERT_NE(limits, nullptr);
  EXPECT_TRUE(WaitUntil([&] { return limits->isVisible(); }))
      << "Inspector limits block stayed hidden: the node's limit bands never "
         "became readable";
}

}  // namespace

TEST_F(ScreenshotGenerator, CaptureMainWindow) {
  // Under --theme the same capture renders the reshelled operator workbench
  // (activity rail, context bar with the severity tiles, editor tabs, status
  // strip) and is published as its own image, so the legacy client-window.png
  // (hand-maintained for the manual until the fake Modus runtime exists) is
  // never overwritten by a themed render.
  const char* filename = GetScreenshotOptions().theme.empty()
                             ? "client-window.png"
                             : "workbench-window.png";
  if (!ShouldCaptureScreenshot(filename))
    GTEST_SKIP() << filename << " not requested";

  MainWindow::SetHideForTesting(false);

  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);
  const auto output_image = output_dir / filename;
  CapturePublishGuard publish_guard{filename};

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

  scada::aui::Tree* tree = nullptr;
  QDockWidget* tree_dock = nullptr;
  // Kept for the selection below: the Inspector reads the *active* view's
  // selection, so the Explorer has to be activated as well as clicked in.
  OpenedView* struct_view = nullptr;
  for (OpenedView* view : main_window.opened_views()) {
    if (view->window_info().name != "Struct")
      continue;

    struct_view = view;
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

  // The Inspector is reshell chrome, so only the themed render has one to
  // fill; the legacy pass renders client-window.png, which has no such panel.
  ASSERT_NE(struct_view, nullptr);
  if (!GetScreenshotOptions().theme.empty()) {
    SelectSignalForInspector(main_window, *struct_view, *tree,
                             app_.node_service(), *qmain);
  }

  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  if (!publish_guard.ShouldPublish()) {
    MainWindow::SetHideForTesting(true);
    return;
  }

  QPixmap pixmap = GrabWhenSettled(qmain);
  pixmap.save(QString::fromStdString(output_image.string()));

  MainWindow::SetHideForTesting(true);
}

// The Overview landing cockpit: a fresh (page-less) profile under the reshell
// theme boots through the production seeding path — BaseMainWindow falls back
// to CreateInitialPage, which returns MakeOverviewPage under the theme — so
// the capture guards the reshell initial-page routing and the page's
// dominant-trend/alarm-strip split, not a hand-assembled page.
TEST_F(ScreenshotGenerator, CaptureOverviewPage) {
  const char* filename = "workbench-overview.png";
  if (GetScreenshotOptions().theme.empty())
    GTEST_SKIP() << "the Overview landing seeds only under the reshell theme";
  if (!ShouldCaptureScreenshot(filename))
    GTEST_SKIP() << filename << " not requested";

  MainWindow::SetHideForTesting(false);

  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);
  const auto output_image = output_dir / filename;
  CapturePublishGuard publish_guard{filename};

  // Deliberately no saved profile: the page-less boot is the state under test.
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_EQ(main_windows.size(), 1u);
  auto& main_window = main_windows.front();

  // The seeded page carries the Overview pair plus the sidebar panes.
  std::set<std::string> view_names;
  for (OpenedView* view : main_window.opened_views())
    view_names.insert(std::string{view->window_info().name});
  EXPECT_TRUE(view_names.contains("Graph"));
  EXPECT_TRUE(view_names.contains("EventJournal"));

  // The sidebar panes really dock rather than opening as workspace tabs: each
  // is a WIN_SING pane, which the view manager routes into a dock widget and
  // tabifies with the others (the dock's tab bar is the pane switcher). This
  // is the runtime half of OverviewPageTest, which can only assert the page
  // composition - the window infos are registered by the running app.
  // The Objects mode's panes, which is what the rail selects by default.
  for (const char* pane : {"Struct", "Portfolio"}) {
    OpenedView* pane_view = nullptr;
    for (OpenedView* view : main_window.opened_views()) {
      if (view->window_info().name == pane) {
        pane_view = view;
        break;
      }
    }
    ASSERT_NE(pane_view, nullptr) << pane << " missing from the Overview page";
    EXPECT_TRUE(pane_view->window_info().is_pane()) << pane;
    ASSERT_NE(pane_view->view(), nullptr) << pane;
    EXPECT_NE(qobject_cast<QDockWidget*>(pane_view->view()->parentWidget()),
              nullptr)
        << pane << " is not docked - it would open as a workspace tab";
  }

  auto* qmain = dynamic_cast<QWidget*>(&main_window);
  ASSERT_NE(qmain, nullptr);

  // The alarm table honours the page's "Current" mode: the journal opens
  // scoped to actionable events — the unacknowledged-only filter pre-set
  // (regression: the mode item was written by every current-events open path
  // but consumed by nothing, so the Overview landed on the full history).
  auto* unacknowledged_only =
      qmain->findChild<QCheckBox*>(QStringLiteral("unacknowledgedOnly"));
  ASSERT_NE(unacknowledged_only, nullptr);
  EXPECT_TRUE(unacknowledged_only->isChecked());
  qmain->resize(1920, 1080);
  qmain->ensurePolished();
  qmain->show();
  scada::screenshot_generator::PumpEventLoopFor(std::chrono::milliseconds(500));

  if (!publish_guard.ShouldPublish()) {
    MainWindow::SetHideForTesting(true);
    return;
  }

  QPixmap pixmap = GrabWhenSettled(qmain);
  pixmap.save(QString::fromStdString(output_image.string()));

  MainWindow::SetHideForTesting(true);
}

// The activity rail on its own — the manual documents it as a surface in its
// own right, and a 1920px window shot cannot show a 52px column legibly.
//
// Captured with several pages so the middle band reads as a group rather than
// as one button, which is the whole point of the band.
TEST_F(ScreenshotGenerator, CaptureActivityRail) {
  constexpr const char* kFilename = "workbench-activity-rail.png";
  if (GetScreenshotOptions().theme.empty())
    GTEST_SKIP() << "the activity rail is reshell chrome, themed runs only";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";

  MainWindow::SetHideForTesting(false);

  auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);
  CapturePublishGuard publish_guard{kFilename};

  {
    Profile profile;
    // Three pages, each with an icon, so the band shows the operator's own
    // glyphs rather than a column of ordinals.
    for (const auto& [title, icon] :
         {std::pair{u"Overview", "overview"}, std::pair{u"Alarms", "alarms"},
          std::pair{u"Trends", "trend"}}) {
      Page page;
      page.title = title;
      page.icon = icon;
      page.AddWindow(WindowDefinition{"Struct"});
      profile.AddPage(page);
    }
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_EQ(main_windows.size(), 1u);
  auto* qmain = dynamic_cast<QWidget*>(&main_windows.front());
  ASSERT_NE(qmain, nullptr);
  qmain->resize(1920, 1080);
  qmain->show();
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  auto* rail = qmain->findChild<QWidget*>("activityBar");
  ASSERT_NE(rail, nullptr) << "the reshell rail is not in the window";

  // The three zones must all be present, or the image documents a rail that
  // is missing one and nothing would say so.
  const QList<QToolButton*> buttons = rail->findChildren<QToolButton*>();
  EXPECT_GE(buttons.size(), 3 + 3 + 1 + 1)
      << "expected pane modes, three pages, the '+' and at least one utility";

  if (!publish_guard.ShouldPublish())
    return;

  const QPixmap frame = GrabWhenSettled(rail);
  ASSERT_FALSE(frame.isNull());
  ASSERT_TRUE(
      frame.save(QString::fromStdString((output_dir / kFilename).string())))
      << "could not write " << kFilename;
}

// The Settings surface, and with it the operator-facing switch for the
// experimental UX themes. Captured in the *default* (untheme'd) run on
// purpose: the operator who needs this image is the one still on Classic,
// looking for how to turn the reshell on.
//
// It grabs the panel the shell builds rather than a form assembled here, for
// the reason the dialog capture did before it: a capture that re-derived the
// preferences would document a surface the client does not ship. The panel is
// an overlay over the main window and takes its rows from the same
// `MainMenuId::Settings` model the menu is built from, so what is grabbed is
// what the operator sees.
//
// The filename still says `dialog`. The surface stopped being one on
// 2026-08-28 and the manual pages that embed this image still describe the
// dialog, so renaming the file here would break those pages before anything
// could fix them — see the manifest note and backlog 628.
TEST_F(ScreenshotGenerator, CaptureSettingsPanel) {
  constexpr const char* kFilename = "settings-dialog.png";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";
  if (!GetScreenshotOptions().theme.empty())
    GTEST_SKIP() << kFilename << " is captured untheme'd only";

  MainWindow::SetHideForTesting(false);

  const auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);
  CapturePublishGuard publish_guard{kFilename};

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));
  for (int i = 0; i < 20; ++i)
    QApplication::processEvents();

  const auto& main_windows = app_.main_window_manager().main_windows();
  ASSERT_EQ(main_windows.size(), 1u);
  auto* qmain = dynamic_cast<QMainWindow*>(&main_windows.front());
  ASSERT_NE(qmain, nullptr);
  qmain->show();

  // The menu now carries one item that opens the surface, so the reachability
  // this used to guard on the Colour scheme submenu is guarded here instead:
  // Colour scheme is how the operator turns the reshell on, and it must not
  // become unreachable.
  QMenuBar* menu_bar = qmain->menuBar();
  ASSERT_NE(menu_bar, nullptr);
  QMenu* settings_menu = nullptr;
  const auto settings_title = QString::fromStdU16String(Translate("Settings"));
  for (QAction* action : menu_bar->actions()) {
    if (action->menu() && action->text() == settings_title)
      settings_menu = action->menu();
  }
  ASSERT_NE(settings_menu, nullptr) << "no Settings menu in the menu bar";
  emit settings_menu->aboutToShow();
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  QAction* open_settings = nullptr;
  const auto item_title = QString::fromStdU16String(Translate("Settings..."));
  for (QAction* action : settings_menu->actions()) {
    if (!action->isSeparator() && action->text() == item_title)
      open_settings = action;
  }
  ASSERT_NE(open_settings, nullptr)
      << "Settings has no Settings... item - the preferences surface, and with "
         "it the appearance switch, is unreachable";

  // Opened the way the menu item and the rail's pinned utility both open it,
  // rather than by constructing a panel here.
  auto* main_window = dynamic_cast<MainWindow*>(&main_windows.front());
  ASSERT_NE(main_window, nullptr);
  main_window->ShowSettings();
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  auto* panel = qmain->findChild<SettingsPanel*>();
  ASSERT_NE(panel, nullptr) << "ShowSettings did not build the panel";
  EXPECT_FALSE(panel->isHidden());

  // Guard the contents, not just that a panel exists: an empty surface would
  // still render a plausible-looking image.
  const QList<QComboBox*> combos = panel->findChildren<QComboBox*>();
  const QList<QCheckBox*> checks = panel->findChildren<QCheckBox*>();
  EXPECT_GE(combos.size(), 1) << "expected Language / Style / Colour scheme";
  EXPECT_GT(checks.size(), 0) << "expected the preference toggles";
  bool has_appearances = false;
  for (const QComboBox* combo : combos) {
    if (combo->count() == 5)
      has_appearances = true;
  }
  EXPECT_TRUE(has_appearances)
      << "no row offers Classic plus the four appearances";

  // The three things that make this a surface rather than the dialog it
  // replaced, and that no other capture in the gallery shows.
  //
  // These also carry weight beyond the image: this is the only place the
  // settings catalogue is exercised against the real `MainMenuModel` and the
  // real registered commands. `client_settings_qt_unittests` drives a
  // `SimpleMenuModel` over a fake delegate, and a fake there was once found
  // reporting a radio group as independent toggles -- a failure no test built
  // on that fake could distinguish from a real one. Measured 2026-08-28: with
  // `BuildSettingsCatalog` stubbed to return nothing, this test and 25 unit
  // tests fail and nothing else in the client suite notices.
  EXPECT_NE(panel->search_field(), nullptr);
  EXPECT_GT(panel->category_list()->count(), 1)
      << "the table of contents lists no categories";
  EXPECT_GT(panel->scope_tabs()->count(), 1)
      << "expected All plus a tab per storage scope";

  // Backlog 554: `Open Displays Folder` is an action the preferences dialog
  // had to skip, and the Displays category on this surface is where it landed.
  // A capture that stopped showing it would be that entry silently reopening.
  const bool has_displays_action =
      std::ranges::any_of(panel->visible_rows(), [](const SettingRow& row) {
        return row.control == SettingControl::kAction;
      });
  EXPECT_TRUE(has_displays_action) << "no action row on the surface";

  // The Language row must agree with the labels around it. It reads the locale
  // back from QSettings, which the fixture pins; without that pin it fell
  // through to the host's system locale and the published image read
  // "Язык: English" beside a form of Russian labels.
  bool language_matches_labels = false;
  const auto russian = QString::fromStdU16String(Translate("Russian"));
  for (const QComboBox* combo : combos) {
    if (combo->currentText() == russian)
      language_matches_labels = true;
  }
  EXPECT_TRUE(language_matches_labels)
      << "Language does not read " << russian.toStdString()
      << " - the capture is showing the host machine's locale, so this image "
         "renders differently depending on who generates it";

  // **A capture that failed its content checks must not publish an image.**
  // Every guard above is `EXPECT` rather than `ASSERT` on purpose, so one bad
  // render reports all of its problems at once instead of stopping at the
  // first — but `EXPECT` does not stop the test, so without this the save ran
  // anyway and a regeneration overwrote the tracked gallery PNG with the very
  // render the assertions had just rejected. Red test, bad file, and the file
  // is what gets committed.
  //
  // An empty settings surface is the case that makes this matter: it lays out
  // perfectly, so nothing about the image looks wrong (`capture.mjs` in the web
  // generator says the same thing about its own empty states, and backlog 583
  // is the Qt instance).
  // What was measured, and what follows rather than being measured: with the
  // catalogue emptied, `client_screenshot_check` fails and writes no
  // `settings-dialog.png` into the build directory it is given. That the
  // *tracked gallery* is likewise protected follows from `--out` being a
  // parameter -- the check passes a build path, `regenerate_client_screenshots`
  // passes `client/screenshots` -- and not from a run, because running
  // regeneration to find out would rewrite ~68 tracked PNGs as macOS renders in
  // a checkout other sessions are working in. Stated this way because the
  // commit that added the guard said "the tracked gallery was untouched
  // throughout", which was true of a run that could not have touched it either
  // way and so evidenced nothing.
  if (!publish_guard.ShouldPublish())
    return;

  QPixmap panel_pixmap = GrabWhenSettled(panel);
  ASSERT_FALSE(panel_pixmap.isNull());
  panel_pixmap.save(QString::fromStdString((output_dir / kFilename).string()));
}

// Menu-popup captures (the `auto-menu` manifest tag).
//
// A `QMenu` renders offscreen without ever being popped up, so these grab the
// menu widget directly rather than driving a click. What they cannot skip is
// populating it: the client's menu bar is model-driven and `BuildMenu` refills
// each menu from its `MenuModel` on every `aboutToShow`, so a menu that has
// never been shown is empty and would still save a plausible-looking image.
// Emitting the signal is exactly what a real click does.
//
// These carry no `screenshot_data.json` spec — they drive the live widget tree
// rather than a fixture view — so `check_screenshots.py` has no dimensions to
// verify for them, and the content assertions below are the whole regression
// net. See "auto-menu — a menu popup" in docs/ops/client-screenshots.md.
namespace {

// Finds the menu-bar menu titled `Translate(english_title)` and populates it.
// Looks the title up through `Translate` rather than by its Russian text: the
// titles have been renamed since the manual's originals were captured (the
// menus the docs call «Далее» and «Схема» now read «Дополнительно» and
// «Мнемосхема»), and the English source string is the stable key.
QMenu* PopulateMenuBarMenu(QMainWindow* qmain, const char* english_title) {
  QMenuBar* menu_bar = qmain->menuBar();
  if (!menu_bar) {
    ADD_FAILURE() << "the main window has no menu bar";
    return nullptr;
  }
  const auto title = QString::fromStdU16String(Translate(english_title));
  QMenu* menu = nullptr;
  for (QAction* action : menu_bar->actions()) {
    if (action->menu() && action->text() == title)
      menu = action->menu();
  }
  if (!menu) {
    ADD_FAILURE() << "no " << english_title << " menu in the menu bar";
    return nullptr;
  }
  emit menu->aboutToShow();
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();
  return menu;
}

// Counts the menu's real rows — separators are `QAction`s too, and a menu of
// nothing but separators would otherwise read as populated.
int CountMenuRows(const QMenu& menu) {
  int rows = 0;
  for (const QAction* action : menu.actions()) {
    if (!action->isSeparator())
      ++rows;
  }
  return rows;
}

// Lays the populated menu out at its natural size and writes it to
// `GetOutputDir() / filename`.
void SaveMenuCapture(QMenu* menu,
                     const char* filename,
                     const CapturePublishGuard& publish_guard) {
  menu->ensurePolished();
  menu->adjustSize();
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  const auto output_dir = GetOutputDir();
  std::filesystem::create_directories(output_dir);

  if (!publish_guard.ShouldPublish())
    return;

  const QPixmap pixmap = GrabWhenSettled(menu);
  ASSERT_FALSE(pixmap.isNull()) << filename << " grabbed an empty pixmap";
  ASSERT_TRUE(
      pixmap.save(QString::fromStdString((output_dir / filename).string())))
      << "could not write " << filename;
}

}  // namespace

// The More menu, whose subject in the manual is the "Export configuration to
// Excel..." row it ends with (dev/excel.md). The rows above it are the
// checkable view toggles, so the capture guards both: the export row must be
// present — it is `admin_only`, so a non-administrator fixture silently drops
// it — and the toggles above it must still be checkable.
TEST_F(ScreenshotGenerator, CaptureMoreMenu) {
  constexpr const char* kFilename = "menu-excel.png";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";
  if (!GetScreenshotOptions().theme.empty())
    GTEST_SKIP() << kFilename << " is captured untheme'd only";

  MainWindow::SetHideForTesting(false);

  // The manual shows this menu open over the object tree, and the view-scoped
  // rows resolve against whatever view is active, so boot the same page.
  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"Struct"});
    profile.AddPage(page);
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));
  CapturePublishGuard publish_guard{kFilename};

  QMainWindow* qmain = ShowMainWindowForMenuCapture(app_);
  ASSERT_NE(qmain, nullptr);

  QMenu* menu = PopulateMenuBarMenu(qmain, "More");
  ASSERT_NE(menu, nullptr);
  EXPECT_GT(CountMenuRows(*menu), 1) << "the More menu rendered empty";

  const auto excel_title =
      QString::fromStdU16String(Translate("Export Configuration to Excel..."));
  int checkable_rows = 0;
  bool has_excel_row = false;
  for (const QAction* action : menu->actions()) {
    if (action->isSeparator())
      continue;
    if (action->text() == excel_title)
      has_excel_row = true;
    if (action->isCheckable())
      ++checkable_rows;
  }
  EXPECT_TRUE(has_excel_row)
      << "the More menu has no " << excel_title.toStdString()
      << " row - it is admin_only, so this capture is running without the "
         "administrator identity the manual's image depicts";
  EXPECT_GT(checkable_rows, 0) << "no checkable view rows above the export row";

  SaveMenuCapture(menu, kFilename, publish_guard);
}

// The Summary view's Function dropdown: the seven aggregate functions, one of
// them checked. This is a toolbar menu rather than a menu-bar one — the
// command toolbar groups the categories it cannot expand behind a
// `QToolButton` (see `MainWindow::CreateToolBar`) — but it populates the same
// way, through its own `aboutToShow`.
//
// Which function is checked comes from the fixture's Summ window, which sets
// no `AggregateType` and so takes the model's default. The manual's original
// shows Minimum; rather than pin a value the fixture does not state, this
// asserts the invariant that matters — the seven rows are all there and
// exactly one of them is checked.
TEST_F(ScreenshotGenerator, CaptureSummaryFunctionMenu) {
  constexpr const char* kFilename = "menu-summary.png";
  if (!ShouldCaptureScreenshot(kFilename))
    GTEST_SKIP() << kFilename << " not requested";
  if (!GetScreenshotOptions().theme.empty())
    GTEST_SKIP() << kFilename << " is captured untheme'd only";

  MainWindow::SetHideForTesting(false);

  // The Function button's rows are the Summary view's own commands, so the
  // menu only resolves its checked state with a Summary view active.
  {
    Profile profile;
    Page page;
    page.AddWindow(WindowDefinition{"Summ"});
    profile.AddPage(page);
    profile.Save();
  }

  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  QMainWindow* qmain = ShowMainWindowForMenuCapture(app_);
  ASSERT_NE(qmain, nullptr);

  // Activate the Summary view explicitly. The toolbar resolves every row
  // through `ResolveViewCommand`, which asks the *active* view — and a window
  // shown offscreen never receives the focus that would make one active, so
  // without this the seven rows exist but resolve no handler, and
  // `UpdateAction` hides them all. The menu still renders: seven invisible rows
  // grab as an empty popup.
  auto& main_window = app_.main_window_manager().main_windows().front();
  OpenedView* summary_view = nullptr;
  for (OpenedView* view : main_window.opened_views()) {
    if (view->window_info().name == std::string_view{"Summ"})
      summary_view = view;
  }
  ASSERT_NE(summary_view, nullptr) << "the Summary view did not open";
  main_window.ActivateView(*summary_view);
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  auto* toolbar = qmain->findChild<QToolBar*>("CommandToolbar");
  ASSERT_NE(toolbar, nullptr) << "no command toolbar in the main window";

  const auto function_title = QString::fromStdU16String(Translate("Function"));
  QMenu* menu = nullptr;
  for (QToolButton* button : toolbar->findChildren<QToolButton*>()) {
    if (button->text() == function_title && button->menu())
      menu = button->menu();
  }
  ASSERT_NE(menu, nullptr)
      << "no " << function_title.toStdString()
      << " tool button on the command toolbar - the aggregate commands are no "
         "longer grouped behind one";

  emit menu->aboutToShow();
  for (int i = 0; i < 10; ++i)
    QApplication::processEvents();

  CapturePublishGuard publish_guard{kFilename};

  // Seven aggregate functions: First, Last, Count, Minimum, Maximum, Sum,
  // Average (see RegisterSummaryCommandActions). A row that stopped being
  // checkable, or a menu that resolved no checked row, would still render.
  int visible_rows = 0;
  int checkable_rows = 0;
  int checked_rows = 0;
  for (const QAction* action : menu->actions()) {
    if (action->isSeparator())
      continue;
    if (action->isVisible())
      ++visible_rows;
    if (action->isCheckable())
      ++checkable_rows;
    if (action->isChecked())
      ++checked_rows;
  }
  EXPECT_EQ(CountMenuRows(*menu), 7) << "expected the seven aggregate rows";
  EXPECT_EQ(visible_rows, 7)
      << "an aggregate row resolved no command handler and was hidden";
  EXPECT_EQ(checkable_rows, 7) << "an aggregate row stopped being checkable";
  EXPECT_EQ(checked_rows, 1)
      << "expected exactly one aggregate function to read as selected";

  SaveMenuCapture(menu, kFilename, publish_guard);
}

// A control review is worth capturing only if it reviews a change. The
// prompt quotes the point's present reading beside the value the command
// would write, and both are rendered by the item's own formatting — so a
// commanded value that formats to the present reading produces a dialog
// showing «Вкл» commanding «Вкл», which is a well-formed PNG of an operator
// being asked to confirm nothing. check_screenshots.py cannot see it: the
// file exists and has the right dimensions.
//
// A discrete item is where this actually bites, because `command_value` is
// the item's raw state while WriteModel labels states by inverting it
// (`get_or(true) ? 0 : 1`). Raw 0 is therefore the *second* label, so the
// obvious reading of "command the other state" is off by one and renders the
// state the point is already in.
TEST_F(ScreenshotGenerator, ControlConfirmationsReviewARealChange) {
  // Straight out of the fixture rather than FixtureConfig().dialogs, which is
  // filtered by the managed-image gate and by --only.
  WaitForAwaitable(executor_, app_.Start());
  ASSERT_TRUE(WaitForPendingNodeLoads(app_.node_service()));

  int reviewed = 0;
  for (const auto& js : FixtureConfig().json.at("dialogs").as_array()) {
    if (js.at("kind").as_string() != "control-confirm")
      continue;
    const std::string filename(js.at("filename").as_string());
    SCOPED_TRACE(filename);

    scada::NodeId node_id = FixtureConfig().dialog_analog_node_id;
    if (const auto* node = js.as_object().if_contains("node"))
      node_id = NodeIdFromScadaString(std::string_view(node->as_string()));
    ASSERT_FALSE(node_id.is_null());
    ASSERT_TRUE(scada::screenshot_generator::FetchNodesResident(
        app_.node_service(), std::span<const scada::NodeId>{&node_id, 1}));

    // Mirrors BuildControlConfirmation's default; the two have to agree or
    // this test reviews a value the capture never renders.
    double commanded = 12.5;
    if (const auto* value = js.as_object().if_contains("command_value"))
      commanded = value->to_number<double>();
    bool second_stage = false;
    if (const auto* stage = js.as_object().if_contains("second_stage"))
      second_stage = stage->as_bool();

    Profile profile;
    auto model = std::make_shared<WriteModel>(
        WriteContext{.executor_ = executor_,
                     .timed_data_service_ = app_.timed_data_service(),
                     .node_id_ = node_id,
                     .profile_ = profile,
                     .manual_ = false});
    // The present reading arrives over TimedDataService, so it lands on a
    // later turn of the loop. Without this the assertion below compares
    // against the get_or default and passes on a dialog that would render
    // wrongly.
    scada::screenshot_generator::PumpEventLoopFor(
        std::chrono::milliseconds{500});

    const std::u16string message =
        model->GetConfirmationMessage(commanded, second_stage);
    // Read the two quoted values back out of the real prompt rather than
    // recomputing them: that is what the reader of the manual sees, and it
    // keeps the check working whichever way WriteModel formats a value.
    auto quoted_after = [&message](std::u16string_view label) {
      const size_t at = message.find(label);
      if (at == std::u16string::npos)
        return std::u16string();
      const size_t from = at + label.size();
      const size_t eol = message.find(u'\n', from);
      std::u16string text =
          message.substr(from, eol == std::u16string::npos ? eol : eol - from);
      const size_t first = text.find_first_not_of(u" \t");
      return first == std::u16string::npos ? std::u16string()
                                           : text.substr(first);
    };
    const std::u16string present = quoted_after(Translate("Present:"));
    const std::u16string command = quoted_after(Translate("Command:"));

    EXPECT_FALSE(present.empty())
        << "the prompt quotes no present reading, so the value never arrived";
    EXPECT_FALSE(command.empty())
        << "the prompt quotes no commanded value, so command_value does not "
           "render for this item";
    EXPECT_NE(present, command)
        << "the capture reviews a no-op: the command formats to the reading "
           "the point already shows ("
        << UtfConvert<char>(present) << ")";
    ++reviewed;
  }
  EXPECT_GT(reviewed, 0) << "the fixture lost every control-confirm capture";
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
      .dialog_analog_node_id = FixtureConfig().dialog_analog_node_id,
      .login_user_list = FixtureConfig().login_user_list};

  int captured = 0;
  const bool themed = !GetScreenshotOptions().theme.empty();
  for (const auto& spec : FixtureConfig().dialogs) {
    // A themed-only spec is the reshell twin of a legacy capture (see
    // workbench-login.png): rendering it without the theme would save the
    // legacy dialog under the reshell name.
    if (spec.themed_only && !themed)
      continue;
    if (CaptureDialog(spec, env))
      ++captured;
  }

  std::cout << "Captured " << captured << "/" << FixtureConfig().dialogs.size()
            << " dialogs to " << output_dir.string() << std::endl;
}
