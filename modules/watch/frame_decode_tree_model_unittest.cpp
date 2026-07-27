#include "modules/watch/frame_decode_tree_model.h"

#include <gtest/gtest.h>

namespace {

FrameDecode TwoLevelDecode() {
  FrameDecode decode;
  decode.nodes.push_back(
      {.name = u"APCI",
       .value = u"I-format",
       .children = {{.name = u"Start", .value = u"0x68", .offset = 0,
                     .length = 1},
                    {.name = u"N(S) send", .value = u"2045", .offset = 2,
                     .length = 2}}});
  return decode;
}

// The offset column is the pane's reason to exist, and it is the only text the
// model composes rather than passes through.
TEST(FrameDecodeTreeModelTest, FormatsSingleOctetAndRangeOffsets) {
  FrameDecodeTreeModel model;
  model.SetDecode(TwoLevelDecode());

  FrameDecodeTreeNode& apci = model.root()->GetChild(0);
  EXPECT_EQ(apci.GetText(0), u"APCI");
  // A group covers no octets of its own, so it claims none.
  EXPECT_EQ(apci.GetText(1), u"");
  EXPECT_EQ(apci.GetText(2), u"I-format");

  EXPECT_EQ(apci.GetChild(0).GetText(1), u"@0");
  EXPECT_EQ(apci.GetChild(1).GetText(1), u"@2-3");
}

// Selecting another row must replace the tree, not append to it.
TEST(FrameDecodeTreeModelTest, SetDecodeReplacesThePreviousFrame) {
  FrameDecodeTreeModel model;
  model.SetDecode(TwoLevelDecode());
  ASSERT_EQ(model.root()->GetChildCount(), 1);

  FrameDecode other;
  other.nodes.push_back({.name = u"APCI", .value = u"S-format"});
  model.SetDecode(other);

  ASSERT_EQ(model.root()->GetChildCount(), 1);
  EXPECT_EQ(model.root()->GetChild(0).GetText(2), u"S-format");
  EXPECT_EQ(model.root()->GetChild(0).GetChildCount(), 0);
}

// Deselection empties the pane rather than leaving the last frame on screen,
// which would read as if it were still selected.
TEST(FrameDecodeTreeModelTest, AnEmptyDecodeClearsTheTree) {
  FrameDecodeTreeModel model;
  model.SetDecode(TwoLevelDecode());

  model.SetDecode({});

  EXPECT_EQ(model.root()->GetChildCount(), 0);
}

// The view draws an expander for anything that claims children, so a leaf must
// not claim them — TreeNode's default says every node has children.
TEST(FrameDecodeTreeModelTest, LeafFieldsHaveNoChildren) {
  FrameDecodeTreeModel model;
  model.SetDecode(TwoLevelDecode());

  FrameDecodeTreeNode& apci = model.root()->GetChild(0);
  EXPECT_TRUE(apci.HasChildren());
  EXPECT_FALSE(apci.GetChild(0).HasChildren());
}

}  // namespace
