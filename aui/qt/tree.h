#pragma once

#include "aui/color.h"
#include "aui/handlers.h"
#include <boost/json.hpp>

#include <QTreeView>
#include <set>
#include <span>
#include <string_view>

class QEvent;

namespace scada::aui {

class TreeModel;

class ItemDelegate;
class TreeModelAdapter;
class TreeProxyModel;

class Tree : public QTreeView {
 public:
  explicit Tree(std::shared_ptr<TreeModel> model);
  ~Tree();

  // Shows or hides the row for the model's single top-level node.
  //
  // Hidden (the default) makes that node the view's root: its children become
  // the top level and no row is drawn for the node itself. That is what an
  // Explorer pane whose whole subject is one subtree wants — the pane already
  // names the node, so its row says the same thing a second time, and the
  // screen mockups draw such a tree with no root row at all
  // (docs/product/ui-mockups/screens/config-workbench.html starts the hardware
  // tree at the device groups under a `Hardware` pane head).
  //
  // The cost is that the node stops being a clickable row, so a view that
  // hides it must keep its commands reachable some other way.
  void SetRootVisible(bool visible);
  void SetHeaderVisible(bool visible);

  // Loads row glyphs from SVG resources, tinted to follow the palette and
  // re-tinted when the theme changes. Index order is the models' "tile index"
  // contract, inherited from the bitmap strips this replaced
  // (docs/client/ux/iconography.md §5.2).
  void LoadGlyphs(std::span<const std::string_view> resource_paths, int size);

  std::vector<void*> GetOrderedNodes(void* root, bool checked) const;

  int GetSelectionSize() const;
  void* GetSelectedNode();
  void SelectNode(void* node);
  void SetSelectionChangedHandler(SelectionChangedHandler handler);

  bool IsExpanded(void* node, bool up_to_root) const;
  void ExpandNode(void* node);
  std::vector<void*> GetChildNodes(void* parent) const;
  void SetExpandedHandler(TreeExpandedHandler handler);

  void StartEditing(void* node);

  void SetShowChecks(bool show);
  void SetCheckedHandler(TreeCheckedHandler handler);
  bool IsChecked(void* node) const;
  void SetChecked(void* node, bool checked);
  void SetCheckedNodes(std::set<void*> nodes);

  void SetRowHeight(int row_height);

  void SetDoubleClickHandler(DoubleClickHandler handler);

  void SetSorted(bool sorted);
  void SetCompareHandler(TreeCompareHandler handler);

  // Filters visible rows to those whose column-0 text contains `text`
  // (case-insensitive); ancestors of a match stay visible so the match keeps
  // its place in the hierarchy. Empty text clears the filter. Only
  // already-fetched rows are considered, so a lazily-loaded subtree is filtered
  // as it is expanded.
  void SetFilterText(const std::u16string& text);

  void SetContextMenuHandler(ContextMenuHandler handler);
  void SetFocusHandler(FocusHandler handler);

  void SetDragHandler(std::vector<std::string> mime_types, DragHandler handler);
  void SetDropHandler(DropHandler handler);

  // Expands every row the first time the model delivers any, then stops
  // watching. `expandAll()` alone is not enough for a tree whose model is
  // filled asynchronously: called at construction it runs against an empty
  // tree and expands nothing, and the rows that arrive afterwards come up
  // collapsed. One-shot on purpose — a later repopulation must not overrule a
  // group the operator has since collapsed.
  void ExpandAllWhenPopulated();

  boost::json::value SaveState() const;
  void RestoreState(const boost::json::value& data);

 protected:
  // QTreeView
  virtual void changeEvent(QEvent* event) override;
  virtual void drawBranches(QPainter* painter,
                            const QRect& rect,
                            const QModelIndex& index) const override;

 private:
  void ApplyThemePalette();

  // Puts `root_visible_` into effect on the view. Re-run on every model reset,
  // which drops the persistent root index a hidden root depends on.
  void ApplyRootVisible();

  // The colour row glyphs are rendered in, from the live palette.
  Color GlyphTint() const;

  void* GetNode(const QModelIndex& index) const;
  QModelIndex GetIndex(void* node, int column_id) const;

  std::unique_ptr<TreeModelAdapter> model_adapter_;
  std::unique_ptr<TreeProxyModel> proxy_model_;

  std::unique_ptr<ItemDelegate> item_delegate_;

  // Held so ExpandAllWhenPopulated can disconnect itself after it fires.
  QMetaObject::Connection populated_connection_;

  // Last SetRootVisible request. A hidden root is held by a
  // QPersistentModelIndex that a model reset kills, so the state has to be
  // remembered and re-applied rather than set once.
  bool root_visible_ = false;

  friend class TreeProxyModel;
};

}  // namespace scada::aui
