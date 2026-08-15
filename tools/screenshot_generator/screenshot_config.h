#pragma once

#include <boost/json/value.hpp>

#include "scada/node_id.h"

#include <filesystem>
#include <string>
#include <vector>

// One cell of a spreadsheet-backed view (`CusTable`), matching a `SheetCell`
// item in a saved `WindowDefinition`. `text` is either a literal label or a
// `"=<formula>"` binding, which is what makes the cell show a live value.
// Coordinates are 1-based, as the profile stores them.
struct SheetCellSpec {
  int row = 0;
  int column = 0;
  std::string text;
  // "left" (default), "center" or "right".
  std::string align;
  // Cell background as `#AARRGGBB`; an alpha of 0 reads as "no colour".
  std::string color;
};

// Configuration for a single main-window view capture. Matches one row
// in the `screenshots:` array of `screenshot_data.json`.
//
// A row carries either `type` or `capture`, never both:
//
//   `type` names a registered `WindowInfo` — the spec becomes a window on the
//   generator's profile page and is grabbed from the opened view.
//
//   `capture` names a standalone capture routine dispatched in the
//   CaptureAllWindows TEST_F, the same way `DialogSpec::kind` is dispatched in
//   `dialog_capture.cpp`. Such a spec never reaches the profile page: most
//   name chrome that has no registered window type at all, and the few that do
//   (the Administration explorer, Roles, the users-admin grid) still need a
//   bespoke fixture — an authenticated identity, or a panel the shell builds
//   only under `--theme`.
struct ScreenshotSpec {
  // Registered window type; empty when `capture` drives this spec.
  std::string window_type;
  // Standalone capture key; empty when `window_type` drives this spec.
  std::string capture;
  std::string filename;
  std::string path;
  // Optional multiple item paths/formulas. Views that hold a list of data
  // items (Table, Watch) seed one row per entry so the capture shows a
  // populated grid instead of an empty view.
  std::vector<std::string> paths;
  // Optional per-item column width, in pixels, for views that store one per
  // data item (Summ, Sheet). 0 leaves the view's own default. The summary's
  // 100 px default truncates a column title down to a fragment of the node's
  // full display path, so a capture whose headers must be readable pins a
  // wider column here rather than shipping elided headers.
  int column_width = 0;
  // Per-column widths, in pixels, for a view that stores them individually
  // (`Sheet`) — entry i sets column i. Use `column_width` when one width fits
  // every column.
  std::vector<int> column_widths;
  // Cells of a spreadsheet-backed view. A `CusTable` capture has nothing to
  // show without them: the view starts as an empty sheet, and its content is
  // whatever the saved window holds.
  std::vector<SheetCellSpec> cells;
  int width = 800;
  int height = 600;
  // Minimum number of grid rows the rendered window must show. 0 disables
  // the check. Set it for grid-backed windows (node tables, transmission)
  // so a data-path regression fails the capture instead of silently saving
  // an empty frame.
  int min_rows = 0;
  // Exact grid row count ("rows" in the JSON): additionally catches rows
  // leaking IN from outside the captured scope, not just an empty grid.
  int exact_rows = 0;
  // Minimum number of grid columns the rendered window must show. 0 disables
  // the check. The row-count checks above cannot see a grid whose columns come
  // from the fixture rather than its rows: the summary renders one column per
  // configured data item over a fixed time axis, so it kept its full set of
  // time rows while showing no data at all, and shipped that way.
  int min_columns = 0;
  // Optional: objectName of a child button to click before grabbing (e.g. a
  // subtab), so a capture can show a non-default tab of a multi-tab view.
  std::string click_object;
  // Expand every row of a tree-backed window before grabbing. A collapsed
  // tree captures its folders and hides everything the capture is about.
  bool expand = false;
};

// Configuration for a single modal-dialog capture. `kind` is the
// dispatch key in `dialog_capture.cpp`.
struct DialogSpec {
  std::string kind;
  std::string filename;
  int width = 0;
  int height = 0;
  // Renders only under --theme ("themed_only" in the JSON). Lets one dialog
  // kind have both a legacy and a reshell capture without either run
  // overwriting the other's image.
  bool themed_only = false;
  // objectName of a QComboBox to drop open before grabbing. The open list is
  // a separate top-level window, so the capture composes it onto the dialog
  // and the saved image is taller than `height`.
  std::string expand_combo;
  // Node this capture operates on ("node" in the JSON), overriding the
  // fixture-wide `dialog_analog_node_id` for the node-driven kinds (limits,
  // write-manual, write-remote, control-confirm). Null means "use the
  // fixture-wide node". Two captures of one kind can differ only in the state
  // of their target — the satisfied and unsatisfied variants of the control
  // dialog's output condition are the same dialog over two nodes — and a
  // single fixture-wide node cannot render both in one run.
  scada::NodeId node_id;
};

// Screenshot-generator fixture, loaded once from `screenshot_data.json`.
// Holds the parsed JSON tree plus the lightweight spec vectors; the
// per-capture files consult one or the other.
struct ScreenshotConfig {
  boost::json::value json;
  std::vector<ScreenshotSpec> screenshots;
  std::vector<DialogSpec> dialogs;
  scada::NodeId dialog_analog_node_id;
  // Accounts the login dialog offers in its user combo ("login_user_list").
  // Fixture data rather than a literal in the capture: these are the operator
  // names the manual's login page shows, and they are Russian — which belongs
  // in the fixture next to the rest of the fixture's Russian, not in client
  // source. The first entry is also seeded as the pre-selected user.
  std::vector<std::string> login_user_list;

  // Reads `path` and populates the fields. Uses `ASSERT_*` on failure
  // so a bad fixture fails the test suite before any TEST_F runs.
  void Load(const std::filesystem::path& path);
};

// Returns true when `filename` is tagged `auto-*` in the image manifest and
// should therefore be rendered by the screenshot generator.
bool IsAutoManagedImageFilename(std::string_view filename);

// Locates `screenshot_data.json` on disk: next to this file, then the
// current working directory. Falls back to a bare filename if neither
// exists.
std::filesystem::path GetDataFilePath();
