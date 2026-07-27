#include "modules/watch/frame_decode_tree_model.h"

#include "aui/translation.h"
#include "base/utf_convert.h"

#include <format>

namespace {

std::u16string FormatOffset(const FrameDecodeNode& node) {
  if (node.length == 0)
    return {};
  if (node.length == 1)
    return UtfConvert<char16_t>(std::format("@{}", node.offset));
  return UtfConvert<char16_t>(
      std::format("@{}-{}", node.offset, node.offset + node.length - 1));
}

}  // namespace

// FrameDecodeTreeNode

FrameDecodeTreeNode::FrameDecodeTreeNode(const FrameDecodeNode& node)
    : name_{node.name}, offset_{FormatOffset(node)}, value_{node.value} {
  for (const FrameDecodeNode& child : node.children)
    Add(GetChildCount(), std::make_unique<FrameDecodeTreeNode>(child));
}

std::u16string FrameDecodeTreeNode::GetText(int column_id) const {
  switch (column_id) {
    case 0: return name_;
    case 1: return offset_;
    case 2: return value_;
    default: return {};
  }
}

// FrameDecodeTreeModel

FrameDecodeTreeModel::FrameDecodeTreeModel() {
  SetDecode({});
}

void FrameDecodeTreeModel::SetDecode(const FrameDecode& decode) {
  TreeModelResetting();
  auto root = std::make_unique<FrameDecodeTreeNode>(FrameDecodeNode{});
  for (const FrameDecodeNode& node : decode.nodes)
    root->Add(root->GetChildCount(), std::make_unique<FrameDecodeTreeNode>(node));
  set_root(std::move(root));
  TreeModelReset();
}

int FrameDecodeTreeModel::GetColumnPreferredSize(int column_id) const {
  // Sized for the widest label the decoder produces at its deepest indent
  // ("Common address" under Object under ASDU) and for a two-ended offset
  // ("@10-11"). Resizing to contents instead does not work here: the tree is
  // built after the view, and QTreeView's content width ignores the indent, so
  // every child row came out elided.
  switch (column_id) {
    case 0: return 180;
    case 1: return 70;
    default: return 0;
  }
}

std::u16string FrameDecodeTreeModel::GetColumnText(int column_id) const {
  switch (column_id) {
    case 0: return Translate("Field");
    case 1: return Translate("Offset");
    case 2: return Translate("Value");
    default: return {};
  }
}
