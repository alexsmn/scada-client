#pragma once

#include "aui/color.h"
#include "aui/models/color_role.h"
#include "aui/models/edit_data.h"
#include "aui/severity_colors.h"

#include <boost/signals2/connection.hpp>
#include <boost/signals2/signal.hpp>
#include <functional>
#include <optional>
#include <string>

namespace scada::aui {

// GetIcon's "this row has no icon" answer. A tree whose rows are told apart by
// indentation and the twisty alone returns this for every node.
inline constexpr int kNoIcon = -1;

// A node's status badge: the quality band it shows and the operator-facing name
// of the state behind it. The adapter draws the band as a colour *and* a shape
// (filled / half / ring with «?») and offers `label` as the row's tooltip, so
// the state never rests on colour alone (docs/client/ux/principles.md §5).
// `label` is the model's because the referent differs per tree: a value's
// quality («Недостоверно») in the Explorer, a device's link state («Нет связи»)
// in the hardware tree.
struct StatusIndicator {
  Quality quality = Quality::kGood;
  std::u16string label;

  bool operator==(const StatusIndicator&) const = default;
};

class TreeModel {
 public:
  using NodeRangeCallback =
      std::function<void(void* parent, int start, int count)>;
  using NodeChangedCallback = std::function<void(void* node)>;
  using ResetCallback = std::function<void()>;

  TreeModel() {}
  virtual ~TreeModel() {}

  TreeModel(const TreeModel&) = delete;
  TreeModel& operator=(const TreeModel&) = delete;

  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodesAdding(
      const NodeRangeCallback& callback) {
    return nodes_adding_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodesAdded(
      const NodeRangeCallback& callback) {
    return nodes_added_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodesDeleting(
      const NodeRangeCallback& callback) {
    return nodes_deleting_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodesDeleted(
      const NodeRangeCallback& callback) {
    return nodes_deleted_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeNodeChanged(
      const NodeChangedCallback& callback) {
    return node_changed_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeModelResetting(
      const ResetCallback& callback) {
    return model_resetting_signal_.connect(callback);
  }
  [[nodiscard]] boost::signals2::scoped_connection SubscribeModelReset(
      const ResetCallback& callback) {
    return model_reset_signal_.connect(callback);
  }

  virtual void* GetRoot() = 0;
  virtual int GetColumnCount() const { return 1; }
  virtual std::u16string GetColumnText(int column_id) const {
    return std::u16string();
  }
  virtual int GetColumnPreferredSize(int column_id) const { return 0; }

  virtual void* GetParent(void* node) = 0;
  virtual int GetChildCount(void* parent) { return 0; }
  virtual void* GetChild(void* parent, int index) { return NULL; }
  virtual std::u16string GetText(void* node, int column_id) {
    return std::u16string();
  }
  virtual int GetIcon(void* node) { return kNoIcon; }
  virtual Color GetTextColor(void* node, int column_id) {
    return ColorCode::Transparent;
  }
  virtual Color GetBackgroundColor(void* node, int column_id) {
    return ColorCode::Transparent;
  }

  // The meaning behind this cell's colouring, for an adapter that can resolve
  // it against the platform palette. Consulted before `GetTextColor` and
  // `GetBackgroundColor`, so a model answers with one or the other and never
  // both. Default: the view's own colours.
  virtual ColorRole GetColorRole(void* node, int column_id) {
    return ColorRole::Default;
  }

  // Optional status badge for a node, drawn before the name column's icon.
  // Default: none, so the node shows no badge.
  virtual std::optional<StatusIndicator> GetStatusIndicator(void* node) {
    return std::nullopt;
  }

  // True when the column's cells hold values or timestamps that should render
  // in the design-system monospace value font under the opt-in token themes
  // (e.g. the Explorer's live Value column), so digits stay tabular as they
  // update. Default: the regular UI font.
  virtual bool IsMonospaceColumn(int column_id) const { return false; }

  virtual void SetText(void* node, int column_id, const std::u16string& text) {}
  // TODO: Remove. `IsEditable` should be covered by `GetEditData` none editor
  // type.
  virtual bool IsEditable(void* node, int column_id) const { return false; }
  virtual bool IsSelectable(void* node, int column_id) const { return true; }
  virtual EditData GetEditData(void* node, int column_id) const { return {}; }
  virtual void HandleEditButton(void* node, int column_id) const {}

  virtual bool HasChildren(void* parent) const { return true; }

  virtual bool CanFetchMore(void* parent) const { return false; }
  virtual void FetchMore(void* parent) {}

 protected:
  void TreeNodesAdding(void* parent, int start, int count);
  void TreeNodesAdded(void* parent, int start, int count);
  void TreeNodesDeleting(void* parent, int start, int count);
  void TreeNodesDeleted(void* parent, int start, int count);
  void TreeNodeChanged(void* node);
  void TreeModelResetting();
  void TreeModelReset();

 private:
  boost::signals2::signal<void(void*, int, int)> nodes_adding_signal_;
  boost::signals2::signal<void(void*, int, int)> nodes_added_signal_;
  boost::signals2::signal<void(void*, int, int)> nodes_deleting_signal_;
  boost::signals2::signal<void(void*, int, int)> nodes_deleted_signal_;
  boost::signals2::signal<void(void*)> node_changed_signal_;
  boost::signals2::signal<void()> model_resetting_signal_;
  boost::signals2::signal<void()> model_reset_signal_;
};

}  // namespace scada::aui
