#include "aui/models/property_tree_model.h"

#include "aui/models/property_model.h"

#include <boost/signals2/connection.hpp>
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace scada::aui {
namespace {

// A minimal in-memory PropertyGroup whose entries carry an explicit EditData,
// which is what decides whether a row is modifiable.
//
// `modules/parameter_form/qt/device_parameter_form_unittest.cpp` has a fake of
// the same shape. The duplication is deliberate: aui is slated for extraction
// into its own repository and may not include client-repo headers, so it
// cannot share a helper out of `client/test/` (see
// docs/client/aui-extraction.md).
class FakeGroup : public PropertyGroup {
 public:
  void AddProperty(std::u16string name,
                   std::u16string value,
                   EditData::EditorType editor_type) {
    entries_.push_back({.name = std::move(name),
                        .value = std::move(value),
                        .edit = EditData{.editor_type = editor_type},
                        .type = ItemType::Property});
  }

  // Adds a nested group and hands back a reference to it, so a test can drive
  // a change against a group that is not the root. The subgroup is owned
  // through a `unique_ptr`, so the reference survives `entries_` reallocating.
  FakeGroup& AddSubgroup(std::u16string name) {
    auto subgroup = std::make_unique<FakeGroup>();
    FakeGroup& ref = *subgroup;
    entries_.push_back({.name = std::move(name),
                        .subgroup = std::move(subgroup),
                        .type = ItemType::Group});
    return ref;
  }

  int GetCount() const override { return static_cast<int>(entries_.size()); }
  PropertyGroup* GetSubgroup(int i) const override {
    return entries_[i].subgroup.get();
  }
  std::u16string GetName(int i) const override { return entries_[i].name; }
  std::u16string GetValue(int i) const override { return entries_[i].value; }
  ItemType GetType(int i) const override { return entries_[i].type; }
  bool IsInherited(int) const override { return false; }
  void SetValue(int i, const std::u16string& value) override {
    entries_[i].value = value;
  }
  EditData GetEditData(int i) const override { return entries_[i].edit; }
  void HandleEditButton(int) const override {}

 private:
  struct Entry {
    std::u16string name;
    std::u16string value;
    EditData edit;
    std::unique_ptr<FakeGroup> subgroup;
    ItemType type = ItemType::Property;
  };

  std::vector<Entry> entries_;
};

class FakeModel : public PropertyModel {
 public:
  explicit FakeModel(std::unique_ptr<FakeGroup> root)
      : root_{std::move(root)} {}

  PropertyGroup& GetRootGroup() override { return *root_; }

 private:
  const std::unique_ptr<FakeGroup> root_;
};

// Index 0 is a writable property, index 1 one the group will not write.
class PropertyTreeModelTest : public testing::Test {
 protected:
  PropertyTreeModelTest() {
    auto group = std::make_unique<FakeGroup>();
    group->AddProperty(u"Name", u"Pump 1", EditData::EditorType::TEXT);
    group->AddProperty(u"NodeClass", u"Object", EditData::EditorType::NONE);
    model_ = std::make_unique<FakeModel>(std::move(group));
    tree_model_ = std::make_unique<PropertyTreeModel>(*model_);
  }

  PropertyItemTreeNode& Item(int index) {
    return static_cast<PropertyItemTreeNode&>(
        tree_model_->root()->GetChild(index));
  }

  static constexpr int kNameColumn = 0;
  static constexpr int kValueColumn = 1;

  std::unique_ptr<FakeModel> model_;
  std::unique_ptr<PropertyTreeModel> tree_model_;
};

TEST_F(PropertyTreeModelTest, WritablePropertyIsEditableAndKeepsDefaultColor) {
  EXPECT_TRUE(Item(0).IsModifiable());
  EXPECT_TRUE(Item(0).IsEditable(kValueColumn));
  EXPECT_EQ(Item(0).GetColorRole(kValueColumn), ColorRole::Default);
}

// The regression: a property the group reports as NONE used to be handed a
// text editor anyway, because IsEditable answered from the column alone. The
// operator typed into a read-only attribute and the edit vanished.
TEST_F(PropertyTreeModelTest, UnwritablePropertyIsNotEditable) {
  EXPECT_FALSE(Item(1).IsModifiable());
  EXPECT_FALSE(Item(1).IsEditable(kValueColumn));
}

// The value cell asks for `Disabled` rather than naming a grey. A literal
// mid-grey ignores the platform theme and reads wrong on a dark palette; the
// Qt adapter resolves this role against `QPalette`, which follows the OS.
TEST_F(PropertyTreeModelTest, UnwritablePropertyValueIsDisabled) {
  EXPECT_EQ(Item(1).GetColorRole(kValueColumn), ColorRole::Disabled);
}

// The models must not answer a role *and* a literal colour -- the adapter
// consults the role first, so a literal left behind here would be dead code
// that looks live.
TEST_F(PropertyTreeModelTest, UnwritablePropertyNamesNoLiteralColour) {
  EXPECT_EQ(Item(1).GetTextColor(kValueColumn), Color{ColorCode::Transparent});
  EXPECT_EQ(Item(1).GetBackgroundColor(kValueColumn),
            Color{ColorCode::Transparent});
}

// The name column is never editable in any row, so greying it would say
// nothing about this row in particular.
TEST_F(PropertyTreeModelTest, NameColumnIsNeitherEditableNorGreyed) {
  for (int index = 0; index < 2; ++index) {
    EXPECT_FALSE(Item(index).IsEditable(kNameColumn));
    EXPECT_EQ(Item(index).GetColorRole(kNameColumn), ColorRole::Default);
  }
}

// A root property and a nested group holding one of its own, which is the
// shape `FindGroupNodeHelper` has to walk.
class NestedPropertyTreeModelTest : public testing::Test {
 protected:
  NestedPropertyTreeModelTest() {
    auto root = std::make_unique<FakeGroup>();
    root->AddProperty(u"Name", u"Pump 1", EditData::EditorType::TEXT);
    FakeGroup& limits = root->AddSubgroup(u"Limits");
    limits.AddProperty(u"EuHi", u"100", EditData::EditorType::TEXT);
    nested_ = &limits;
    root_group_ = root.get();
    model_ = std::make_unique<FakeModel>(std::move(root));
    tree_model_ = std::make_unique<PropertyTreeModel>(*model_);
    connection_ = tree_model_->SubscribeNodeChanged(
        [this](void* node) { changed_.push_back(node); });
  }

  static constexpr int kNameColumn = 0;

  FakeGroup* nested_ = nullptr;
  FakeGroup* root_group_ = nullptr;
  std::vector<void*> changed_;
  std::unique_ptr<FakeModel> model_;
  std::unique_ptr<PropertyTreeModel> tree_model_;
  boost::signals2::scoped_connection connection_;
};

// The regression: `FindGroupNodeHelper` ignored the `parent` it was handed and
// re-derived the root instead, and its loop body did not depend on the loop
// variable -- so no child was ever examined. A change inside a subgroup found
// no node, `PropertiesChanged` returned early, and the nested row kept its
// stale text until something repopulated the whole tree.
TEST_F(NestedPropertyTreeModelTest, ChangeInNestedGroupReachesTheNestedNode) {
  model_->properties_changed_handler(*nested_, 0, 1);

  PropertyTreeNode& nested_node = tree_model_->root()->GetChild(1);
  ASSERT_EQ(nested_node.GetChildCount(), 1);
  ASSERT_EQ(changed_.size(), 1u);
  EXPECT_EQ(changed_[0], &nested_node.GetChild(0));
}

// The root-level path worked before only because the old loop happened to
// match on its first iteration; keep it covered now that the walk is real.
TEST_F(NestedPropertyTreeModelTest, ChangeInRootGroupReachesTheRootRow) {
  model_->properties_changed_handler(*root_group_, 0, 1);

  ASSERT_EQ(changed_.size(), 1u);
  EXPECT_EQ(changed_[0], &tree_model_->root()->GetChild(0));
}

// A group that is not in this tree must find nothing rather than reporting the
// root, which is what the old helper did whenever the root happened to match.
TEST_F(NestedPropertyTreeModelTest, ChangeInAnUnrelatedGroupEmitsNothing) {
  FakeGroup stranger;
  stranger.AddProperty(u"EuLo", u"0", EditData::EditorType::TEXT);

  model_->properties_changed_handler(stranger, 0, 1);

  EXPECT_TRUE(changed_.empty());
}

// A category row heads the rows under it and used to name white-on-grey
// outright, which ignored the platform theme. It now asks for `Header` and
// lets the Qt adapter take the heading colours from `QPalette`. A plain
// group is not a heading and keeps the view's own colours.
TEST_F(NestedPropertyTreeModelTest, CategoryAsksForTheHeaderRole) {
  PropertyGroupTreeNode* root_node = tree_model_->root()->AsGroup();
  ASSERT_NE(root_node, nullptr);
  EXPECT_EQ(root_node->type, PropertyGroup::ItemType::Category);
  EXPECT_EQ(root_node->GetColorRole(kNameColumn), ColorRole::Header);

  PropertyGroupTreeNode* group_node =
      tree_model_->root()->GetChild(1).AsGroup();
  ASSERT_NE(group_node, nullptr);
  EXPECT_EQ(group_node->type, PropertyGroup::ItemType::Group);
  EXPECT_EQ(group_node->GetColorRole(kNameColumn), ColorRole::Default);
}

// Same reasoning as the property rows: a role and a literal colour must not
// both be answered, or the adapter's precedence hides one of them.
TEST_F(NestedPropertyTreeModelTest, CategoryNamesNoLiteralColour) {
  PropertyGroupTreeNode* root_node = tree_model_->root()->AsGroup();
  ASSERT_NE(root_node, nullptr);
  EXPECT_EQ(root_node->GetTextColor(kNameColumn),
            Color{ColorCode::Transparent});
  EXPECT_EQ(root_node->GetBackgroundColor(kNameColumn),
            Color{ColorCode::Transparent});
}

}  // namespace
}  // namespace scada::aui
