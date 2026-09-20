#pragma once

#include "base/any_executor.h"

class MainWindow;
class NodeService;
class OpenedView;
class QDockWidget;
class QWidget;

namespace scada::aui {
class Tree;
}

namespace scada::screenshot_generator {

// Expands and settles the Explorer tree so the capture grabs laid-out rows
// rather than an empty viewport. The caller frames the dock first; `tree_dock`
// is taken only so a failure can report whether it was visible.
//
// This is more than an expand because the rows are lazily loaded through a
// proxy: a level has to be fetched and settled before the next one exists, and
// Qt can land in an "expanded root with empty child viewport" state that only a
// collapse/expand cycle clears. Reports and returns false rather than leaving a
// blank tree in the image — a tree that renders no rows still saves a
// well-formed PNG of exactly the spec's dimensions.
bool MaterializeExplorerTree(scada::aui::Tree& tree,
                             QDockWidget* tree_dock,
                             AnyExecutor executor,
                             NodeService& node_service);

// Puts a selection on screen for the workbench captures, because the Inspector
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
                              AnyExecutor executor,
                              NodeService& node_service,
                              QWidget& window);

}  // namespace scada::screenshot_generator
