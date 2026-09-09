#include "view_capture.h"

#include "graph_capture.h"
#include "publish_guard.h"
#include "screenshot_wait.h"
#include "widget_capture.h"

#include "aui/tree.h"
#include "controller/window_info.h"
#include "main_window/main_window.h"
#include "main_window/opened_view/opened_view.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"

#include <gtest/gtest.h>

#include <QtCore/QElapsedTimer>
#include <QtWidgets/QAbstractButton>
#include <QtWidgets/QApplication>
#include <QtWidgets/QTableView>

#include <algorithm>
#include <chrono>
#include <ranges>
#include <vector>

namespace scada::screenshot_generator {

namespace {

// Pulls the data items a spec names (`path` plus `paths`) fully resident, so a
// capture does not depend on another window in the same run having browsed
// them first. Non-node paths (formulas, unresolvable ids) simply yield no node
// id and are skipped.
void MakeSpecItemsResident(AnyExecutor executor,
                           NodeService& node_service,
                           const ScreenshotSpec& spec) {
  std::vector<scada::NodeId> node_ids;
  auto add = [&node_ids](const std::string& path) {
    if (path.empty())
      return;
    scada::NodeId node_id = NodeIdFromScadaString(path);
    if (!node_id.is_null())
      node_ids.push_back(std::move(node_id));
  };

  add(spec.path);
  for (const auto& path : spec.paths)
    add(path);
  // A spreadsheet names its items nowhere else: its live cells are
  // "=<formula>" text. Without warming them the sheet renders a raw value
  // where a fuller run — one where another window happened to browse the same
  // node — renders the item's display format.
  for (const auto& cell : spec.cells) {
    if (cell.text.starts_with('='))
      add(cell.text.substr(1));
  }

  if (node_ids.empty())
    return;

  scada::screenshot_generator::FetchNodesResident(executor, node_service,
                                                  node_ids);
}

// The grid widget a view renders into, itself or the first one below it.
QTableView* FindGridWidget(QWidget* widget) {
  if (!widget)
    return nullptr;
  if (auto* table = qobject_cast<QTableView*>(widget))
    return table;
  return widget->findChild<QTableView*>();
}

// Cells of a grid model whose display text is not empty.
int CountFilledCells(const QAbstractItemModel& model) {
  int filled = 0;
  for (int row = 0; row < model.rowCount(); ++row) {
    for (int column = 0; column < model.columnCount(); ++column) {
      if (!model.index(row, column).data(Qt::DisplayRole).toString().isEmpty())
        ++filled;
    }
  }
  return filled;
}

// Rows a tree model has materialized under `parent`, descendants included.
// Reads `rowCount` only — a lazy tree fetches through `canFetchMore`/
// `fetchMore`, so counting never pulls in rows the capture would not show.
int CountLoadedRows(const QAbstractItemModel& model,
                    const QModelIndex& parent) {
  const int count = model.rowCount(parent);
  int total = count;
  for (int row = 0; row < count; ++row)
    total += CountLoadedRows(model, model.index(row, 0, parent));
  return total;
}

}  // namespace

scada::aui::Tree* FindTreeWidget(QWidget* widget) {
  if (!widget)
    return nullptr;

  if (auto* tree = dynamic_cast<scada::aui::Tree*>(widget))
    return tree;

  for (auto* child : widget->findChildren<QWidget*>()) {
    if (auto* tree = dynamic_cast<scada::aui::Tree*>(child))
      return tree;
  }

  return nullptr;
}

bool CaptureViewSpec(const ScreenshotSpec& spec,
                     const ViewCaptureContext& context,
                     std::set<const OpenedView*>& used_views) {
  // Built first, so it precedes every row/column/fill assertion below. This
  // is the whole sweep's guard: CaptureAllWindows renders dozens of specs
  // inside one TEST_F, and each spec's content checks live here.
  CapturePublishGuard publish_guard{spec.filename};

  // A sidebar pane is only on screen while its activity-rail mode is
  // selected — the rail is authoritative over the left dock (see
  // main_window/pane_modes.h). Select the owning mode first, exactly as an
  // operator would, so the pane exists to be captured.
  if (context.main_window.SelectPaneModeForPane(spec.window_type)) {
    // The switch destroyed the previous mode's panes. `used_views` keys on
    // raw pointers, and a freshly created view can land on a freed address,
    // so a stale entry would make the new pane look already-consumed. The
    // set only disambiguates specs that share a window_type within one
    // layout, so dropping it at a mode boundary loses nothing.
    used_views.clear();
  }

  OpenedView* view = nullptr;
  for (OpenedView* v : context.main_window.opened_views()) {
    if (v->window_info().name == spec.window_type && !used_views.contains(v)) {
      view = v;
      break;
    }
  }

  if (!view) {
    ADD_FAILURE() << "Window type not found: " << spec.window_type;
    return false;
  }
  used_views.insert(view);

  QWidget* widget = view->view();
  if (!widget) {
    ADD_FAILURE() << "No QWidget for: " << spec.window_type;
    return false;
  }

  // Make this spec's own items fully resident before grabbing. A row's
  // display format, engineering units and limit bands are property children
  // fetched on demand, and TimedData only pulls the item node itself
  // (NodeOnly) — so a row renders unformatted until something else browses
  // those children. In a full run the Explorer tree does that incidentally,
  // which made a capture's correctness depend on which *other* windows the
  // run happened to include: `--only table.png` rendered Частота as "50"
  // where a wider run rendered the fixture's "0.00" format as "50.01".
  // Warming the spec's own nodes makes each capture self-sufficient.
  MakeSpecItemsResident(context.executor, context.node_service, spec);

  // Those fetches can in turn start history reads (a row's alias resolves,
  // then asks for its sparkline window), so settle again before grabbing.
  EXPECT_TRUE(scada::screenshot_generator::WaitForPendingData(
      context.executor, context.node_service, context.timed_data_service))
      << spec.filename;

  // A collapsed tree captures its folders and hides everything the capture
  // is about, so a tree-backed spec can ask for every row. Done before the
  // row check below, which must count what the capture will actually show:
  // a lazy tree has only materialized its root until something expands it.
  if (spec.expand) {
    if (scada::aui::Tree* tree = FindTreeWidget(widget)) {
      tree->expandAll();
      QApplication::processEvents();
      // expandAll fetches the next level lazily, so each newly shown row can
      // start its own child browse; settle those before counting or grabbing.
      EXPECT_TRUE(
          WaitForPendingNodeLoads(context.executor, context.node_service))
          << spec.filename;
      tree->expandAll();
      QApplication::processEvents();
    } else {
      ADD_FAILURE() << spec.filename << ": expand was requested but "
                    << spec.window_type << " has no tree";
    }
  }

  // A grid-backed window that renders fewer rows than the fixture defines
  // is a data-path regression (empty users/transmission tables have
  // shipped as "successful" captures before) — fail loudly instead of
  // silently saving a bare frame.
  if (spec.min_rows > 0 || spec.exact_rows > 0 || spec.min_columns > 0) {
    int max_rows = 0;
    int max_columns = 0;
    const auto count_grid = [&] {
      max_rows = 0;
      max_columns = 0;
      QList<QTableView*> tables = widget->findChildren<QTableView*>();
      if (auto* table = qobject_cast<QTableView*>(widget))
        tables.prepend(table);
      for (const QTableView* table : tables) {
        if (table->model()) {
          max_rows = std::max(max_rows, table->model()->rowCount());
          max_columns = std::max(max_columns, table->model()->columnCount());
        }
      }
      // Tree-backed windows count every row they have materialized, not just
      // the top level. Without this a tree capture can go empty as silently
      // as a grid one — which is exactly how the favourites pane shipped
      // blank — and a top-level-only count says nothing about a tree whose
      // content sits below its top row.
      //
      // Counted from the view's root index, so the row for a hidden root is
      // not counted — which every Explorer tree now has. A pane whose store
      // is empty therefore counts 0 rather than the 1 its root row used to
      // contribute, and that is the honest number: the capture would be
      // blank.
      if (const scada::aui::Tree* tree = FindTreeWidget(widget)) {
        if (tree->model())
          max_rows = std::max(
              max_rows, CountLoadedRows(*tree->model(), tree->rootIndex()));
      }
    };

    // A panel that fills itself from its own async read is not populated at
    // this point, and nothing above waits for it: WaitForPendingData only
    // sees work the node service has already been asked for, and a view that
    // CoSpawns its read at construction has not necessarily issued it yet
    // when we get here. The Roles panel does exactly that
    // (client/modules/user_access/qt/roles_view.cpp) and counted 0 rows
    // against a fixture that defines two — failing the check AND saving an
    // empty capture, since the grab below happens after this.
    //
    // So settle before judging: pump until the spec's own expectation is
    // met, then assert. A genuinely empty grid still fails, one second
    // later. This cannot paper over a row *leak* (the exact_rows case), for
    // which the wait stops at the first satisfying count and the assertion
    // below is unchanged.
    // PumpEventLoopFor, not the processEvents-based WaitUntil above: the
    // population lands as a MessageLoopQt-scheduled continuation, and on
    // macOS the processEvents forms do not fire timers on a drained queue
    // (see screenshot_wait.h).
    constexpr int kGridSettleTimeoutMs = 10'000;
    QElapsedTimer settle;
    settle.start();
    for (;;) {
      count_grid();
      const bool satisfied =
          (spec.min_rows == 0 || max_rows >= spec.min_rows) &&
          (spec.exact_rows == 0 || max_rows == spec.exact_rows) &&
          (spec.min_columns == 0 || max_columns >= spec.min_columns);
      if (satisfied || settle.elapsed() >= kGridSettleTimeoutMs)
        break;
      scada::screenshot_generator::PumpEventLoopFor(
          std::chrono::milliseconds{50});
    }

    if (spec.min_rows > 0) {
      EXPECT_GE(max_rows, spec.min_rows)
          << spec.filename << ": the " << spec.window_type
          << " grid rendered fewer rows than the fixture populates - the "
             "capture would be empty or partial";
    }
    // The exact expectation additionally catches rows leaking IN from
    // outside the captured scope (the transmission grid once picked up
    // another device's rules from a global model-change event).
    if (spec.exact_rows > 0) {
      EXPECT_EQ(max_rows, spec.exact_rows)
          << spec.filename << ": the " << spec.window_type
          << " grid row count does not match the fixture";
    }
    // A column-per-item grid (the summary) goes empty without losing a
    // single row, so the row checks above cannot see it.
    if (spec.min_columns > 0) {
      EXPECT_GE(max_columns, spec.min_columns)
          << spec.filename << ": the " << spec.window_type
          << " grid rendered fewer columns than the fixture configures - the "
             "capture would show its axis and no data";
    }
  }

  // A spreadsheet is a fixed grid of mostly-empty cells, so neither its row
  // nor its column count moves when its content goes missing — an empty
  // sheet and a full one are the same shape. Count the cells that actually
  // rendered text instead, against the ones the fixture writes. A cell bound
  // to a live value ("=<formula>") that resolves to nothing counts as
  // missing, which is the point.
  if (!spec.cells.empty()) {
    const auto expected = std::ranges::count_if(
        spec.cells,
        [](const SheetCellSpec& cell) { return !cell.text.empty(); });
    int filled = 0;
    if (auto* table = FindGridWidget(widget)) {
      if (const QAbstractItemModel* model = table->model())
        filled = CountFilledCells(*model);
    }
    EXPECT_GE(filled, expected)
        << spec.filename << ": the " << spec.window_type << " sheet rendered "
        << filled << " non-empty cells, fewer than the " << expected
        << " the fixture writes - the capture would be a blank "
           "grid";
  }

  // Optionally click a named child (e.g. a subtab button) so the capture
  // shows a non-default tab of a multi-tab view. Done before SaveScreenshot
  // detaches the widget for the grab.
  if (!spec.click_object.empty()) {
    // The subtab buttons are built when the reshell form finishes its async
    // node browse, which may not have completed yet for a just-opened view;
    // wait for the named button before clicking.
    QAbstractButton* button = nullptr;
    WaitUntil([&] {
      button = widget->findChild<QAbstractButton*>(
          QString::fromStdString(spec.click_object));
      return button != nullptr;
    });
    if (button) {
      button->click();
      QApplication::processEvents();
    } else {
      ADD_FAILURE() << "click_object not found: " << spec.click_object << " in "
                    << spec.window_type;
    }
  }

  if (!publish_guard.ShouldPublish())
    return false;

  if (spec.window_type == "Graph") {
    // Render graph standalone — hidden main windows don't lay out
    // QSplitter children, so we build a fresh graph widget.
    SaveGraphScreenshot(spec, context.executor, context.node_service,
                        context.timed_data_service, context.json);
  } else {
    SaveScreenshot(widget, spec);
  }
  return true;
}

}  // namespace scada::screenshot_generator
