#include "aui/models/property_tree_model.h"

#include "aui/models/property_model.h"

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
    entries_.push_back({std::move(name), std::move(value),
                        EditData{.editor_type = editor_type}});
  }

  int GetCount() const override { return static_cast<int>(entries_.size()); }
  PropertyGroup* GetSubgroup(int) const override { return nullptr; }
  std::u16string GetName(int i) const override { return entries_[i].name; }
  std::u16string GetValue(int i) const override { return entries_[i].value; }
  ItemType GetType(int) const override { return ItemType::Property; }
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
  EXPECT_EQ(Item(0).GetTextColor(kValueColumn), Color{ColorCode::Transparent});
}

// The regression: a property the group reports as NONE used to be handed a
// text editor anyway, because IsEditable answered from the column alone. The
// operator typed into a read-only attribute and the edit vanished.
TEST_F(PropertyTreeModelTest, UnwritablePropertyIsNotEditable) {
  EXPECT_FALSE(Item(1).IsModifiable());
  EXPECT_FALSE(Item(1).IsEditable(kValueColumn));
}

TEST_F(PropertyTreeModelTest, UnwritablePropertyValueIsGrey) {
  EXPECT_EQ(Item(1).GetTextColor(kValueColumn), Color{ColorCode::Gray});
}

// The name column is never editable in any row, so greying it would say
// nothing about this row in particular.
TEST_F(PropertyTreeModelTest, NameColumnIsNeitherEditableNorGreyed) {
  for (int index = 0; index < 2; ++index) {
    EXPECT_FALSE(Item(index).IsEditable(kNameColumn));
    EXPECT_EQ(Item(index).GetTextColor(kNameColumn),
              Color{ColorCode::Transparent});
  }
}

}  // namespace
}  // namespace scada::aui
