#pragma once

#include "aui/models/tree_node_model.h"
#include "modules/watch/frame_decode.h"

// One row of the decode pane: a field or a group, its byte range, and its
// value. Built from a FrameDecodeNode and immutable thereafter — a new
// selection rebuilds the tree rather than editing it.
class FrameDecodeTreeNode : public scada::aui::TreeNode<FrameDecodeTreeNode> {
 public:
  explicit FrameDecodeTreeNode(const FrameDecodeNode& node);

  // scada::aui::TreeNode
  virtual std::u16string GetText(int column_id) const override;
  // Without this every leaf field would draw an expander.
  virtual bool HasChildren() const override { return GetChildCount() != 0; }

 private:
  const std::u16string name_;
  // The byte range, formatted as the mockup shows it: "@6" for one octet,
  // "@2-3" for a range, empty for a node with no octets of its own.
  const std::u16string offset_;
  const std::u16string value_;
};

// The decode-pane tree: field, byte offset, value.
class FrameDecodeTreeModel
    : public scada::aui::TreeNodeModel<FrameDecodeTreeNode> {
 public:
  FrameDecodeTreeModel();

  // Replaces the whole tree with the decode of the newly selected frame.
  void SetDecode(const FrameDecode& decode);

  // scada::aui::TreeModel
  virtual int GetColumnCount() const override { return 3; }
  virtual std::u16string GetColumnText(int column_id) const override;
  virtual int GetColumnPreferredSize(int column_id) const override;
  virtual bool IsMonospaceColumn(int column_id) const override {
    // Offsets and values are measurements: tabular digits keep them aligned
    // between frames.
    return column_id != 0;
  }
};
