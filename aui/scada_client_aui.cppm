// scada.client.aui — named C++20 module facade over the aui headers.
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md): headers stay the source of truth, the global
// module fragment includes them, the purview re-exports names with
// `export using`. `export import scada.base` mirrors aui's sole PUBLIC link
// (scada_base); aui deliberately has no other project dependency — it is
// slated for extraction into its own repository (see docs/aui-extraction.md).
//
// The facade covers the Qt flavor (compiled with aui_qt's flags, so UI_QT is
// defined); the wt flavor stays header-only and is not facaded. The
// platform-agnostic wrapper headers (grid.h, table.h, tree.h, key_codes.h,
// color.h) transitively pull their aui/qt/ implementations — the aui-owned
// names they define (aui::Grid, aui::Table, aui::Tree, aui::Color, ...) ARE
// exported; Qt names (QWidget, QRect, QColor, Qt::*) and the aui/qt adapter
// classes (GridModelAdapter, TableModelAdapter, TreeModelAdapter,
// ItemDelegate, TreeProxyModel) are NOT.
//
// Not included (documented exclusions):
//  - aui/qt/*.h and aui/wt/*.h directly (platform implementation layers;
//    reached only transitively through the wrapper headers above);
//  - color_win.h (Windows-only; color.h self-includes it under _WIN32);
//  - rect_internal.h (internal implementation header, aui::internal, wt-only
//    include path);
//  - dialog_service_mock.h, models/status_bar_model_mock.h,
//    models/tree_model_mock.h (test mocks);
//  - graph.h, view_manager.h: thin wrappers over the external graph_qt /
//    view_manager_qt component libraries (declared aui_qt deps). The names
//    they surface (views::*, ViewManagerQtComponent) belong to those
//    libraries, not aui — include textually where used;
//  - os_exchange_data.h on Windows: the _WIN32 branch declares COM members
//    (IDataObject, FORMATETC, ...) and presumes the consumer included the
//    COM headers first; the portable branch is facaded on non-Windows.
//
// Not exported (textual #include alongside the import where needed):
//  - aui::ShiftModifier / aui::ControlModifier / aui::AltModifier
//    (key_codes.h): namespace-scope non-inline constexpr objects — internal
//    linkage, ill-formed to export; include aui/key_codes.h for them.
//
// No std::hash / std::formatter specializations exist in these headers, so
// no static_assert keep-alives are needed.

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "aui/color.h"
#include "aui/dialog_service.h"
#include "aui/drag_drop_types.h"
#include "aui/grid.h"
#include "aui/handlers.h"
#include "aui/key_codes.h"
#include "aui/models/edit_data.h"
#include "aui/models/fixed_row_model.h"
#include "aui/models/grid_model.h"
#include "aui/models/grid_model_util.h"
#include "aui/models/grid_range.h"
#include "aui/models/header_model.h"
#include "aui/models/menu_model.h"
#include "aui/models/menu_model_delegate.h"
#include "aui/models/menu_separator_types.h"
#include "aui/models/mirror_table_model.h"
#include "aui/models/property_model.h"
#include "aui/models/property_tree_model.h"
#include "aui/models/simple_menu_model.h"
#include "aui/models/status_bar_model.h"
#include "aui/models/table_column.h"
#include "aui/models/table_model.h"
#include "aui/models/tree_model.h"
#include "aui/models/tree_node_model.h"
#ifndef _WIN32
#include "aui/os_exchange_data.h"
#endif
#include "aui/point.h"
#include "aui/prompt_dialog.h"
#include "aui/rect.h"
#include "aui/resource_error.h"
#include "aui/size.h"
#include "aui/table.h"
#include "aui/translation.h"
#include "aui/tree.h"
#include "aui/types.h"
#include "aui/types_fwd.h"

export module scada.client.aui;

// Mirror aui's PUBLIC link transitivity (scada_base only).
export import scada.base;

// ---- namespace aui ----
export namespace aui {

// color.h (aui::Color itself comes from aui/qt/color_qt.h behind UI_QT)
using aui::Color;
using aui::ColorCode;
using aui::ColorToString;
using aui::FindColor;
using aui::FindColorName;
using aui::GetColor;
using aui::GetColorCount;
using aui::GetColorDebugName;
using aui::GetColorName;
using aui::Rgba;
using aui::StringToColor;

// drag_drop_types.h
using aui::DragDropTypes;

// grid.h (wrapper over aui/qt/grid.h; the adapter/delegate classes it pulls
// in are implementation-layer and deliberately not exported)
using aui::Grid;

// key_codes.h (the *Modifier constants have internal linkage — include-only)
using aui::KeyCode;
using aui::KeyModifier;
using aui::KeyModifiers;

// models/edit_data.h
using aui::EditData;

// models/fixed_row_model.h
using aui::FixedRowModel;

// models/grid_model.h
using aui::GridModel;
using aui::GridModelIndex;

// models/grid_model_util.h
using aui::ExpandGridRange;

// models/grid_range.h
using aui::GridRange;

// models/header_model.h
using aui::ColumnHeaderModel;
using aui::HeaderModel;

// models/menu_model.h / models/menu_model_delegate.h
using aui::MenuModel;
using aui::MenuModelDelegate;

// models/menu_separator_types.h (unscoped enum: the enumerators are
// namespace-scope names of their own and are exported individually)
using aui::LOWER_SEPARATOR;
using aui::MenuSeparatorType;
using aui::NORMAL_SEPARATOR;
using aui::SPACING_SEPARATOR;
using aui::UPPER_SEPARATOR;

// models/mirror_table_model.h
using aui::MirrorTableModel;

// models/property_model.h
using aui::PropertyGroup;
using aui::PropertyModel;

// models/property_tree_model.h
using aui::PropertyGroupTreeNode;
using aui::PropertyItemTreeNode;
using aui::PropertyTreeModel;
using aui::PropertyTreeNode;

// models/simple_menu_model.h
using aui::SimpleMenuModel;

// models/status_bar_model.h
using aui::StatusBarModel;

// models/table_column.h
using aui::GridCell;
using aui::TableColumn;

// models/table_model.h
using aui::TableCell;
using aui::TableModel;

// models/tree_model.h
using aui::TreeModel;

// models/tree_node_model.h
using aui::TreeNode;
using aui::TreeNodeModel;
using aui::TreeNodeWithValue;

#ifndef _WIN32
// os_exchange_data.h (portable branch; on Windows the header is include-only
// — see the file comment)
using aui::OSExchangeData;
#endif

// point.h / rect.h / size.h (aui-owned aliases to Qt types under UI_QT; the
// alias names are aui's and exportable, the Qt targets stay include-only)
using aui::Point;
using aui::Rect;
using aui::Size;

// table.h / tree.h (wrappers over aui/qt/table.h, aui/qt/tree.h)
using aui::Table;
using aui::Tree;

// Free operator set at aui namespace scope (color.h: ostream << for Color).
using aui::operator<<;

}  // namespace aui

// ---- global namespace ----
export {
  // dialog_service.h
  using ::DialogService;
  using ::MessageBoxMode;
  using ::MessageBoxResult;

  // handlers.h
  using ::ContextMenuHandler;
  using ::DoubleClickHandler;
  using ::DragData;
  using ::DragHandler;
  using ::DropAction;
  using ::DropHandler;
  using ::FocusHandler;
  using ::KeyPressHandler;
  using ::SelectionChangedHandler;
  using ::SelectionChangeHandler;
  using ::StateChangeHandler;
  using ::TreeCheckedHandler;
  using ::TreeCompareHandler;
  using ::TreeDragHandler;
  using ::TreeEditHandler;
  using ::TreeExpandedHandler;

  // prompt_dialog.h
  using ::RunPromptDialog;

  // resource_error.h
  using ::CatchResourceError;
  using ::GetResourceErrorMessage;
  using ::HandleResourceError;
  using ::HandleResourceErrorAsync;
  using ::ResourceError;
  using ::RethrowResourceError;
  using ::ShowResourceError;
  using ::ShowResourceErrorAsync;

  // translation.h
  using ::Translate;

  // types_fwd.h (types.h completes UiView's target type)
  using ::UiView;
}  // export
