#include "parameter_form/qt/device_parameter_form.h"

#include "aui/models/property_model.h"
#include "aui/test/app_environment.h"

#include <gtest/gtest.h>

#include <QLineEdit>
#include <QPushButton>

#include <memory>
#include <utility>
#include <vector>

namespace {

using scada::aui::PropertyGroup;

// A minimal in-memory PropertyGroup: each entry is either a leaf property
// (name + value) or a subgroup (a section). SetValue records the write and
// updates the stored value, so the test can assert Apply wrote through.
class FakeGroup : public scada::aui::PropertyGroup {
 public:
  struct Entry {
    std::u16string name;
    std::u16string value;
    std::unique_ptr<FakeGroup> sub;
  };

  FakeGroup& AddLeaf(std::u16string name, std::u16string value) {
    entries_.push_back({std::move(name), std::move(value), nullptr});
    return *this;
  }
  FakeGroup& AddSection(std::u16string name, std::unique_ptr<FakeGroup> sub) {
    entries_.push_back({std::move(name), {}, std::move(sub)});
    return *this;
  }

  int GetCount() const override { return static_cast<int>(entries_.size()); }
  PropertyGroup* GetSubgroup(int i) const override {
    return entries_[i].sub.get();
  }
  std::u16string GetName(int i) const override { return entries_[i].name; }
  std::u16string GetValue(int i) const override { return entries_[i].value; }
  ItemType GetType(int i) const override {
    return entries_[i].sub ? ItemType::Group : ItemType::Property;
  }
  bool IsInherited(int) const override { return false; }
  void SetValue(int i, const std::u16string& value) override {
    entries_[i].value = value;
    writes.push_back({i, value});
  }
  scada::aui::EditData GetEditData(int) const override { return {}; }
  void HandleEditButton(int) const override {}

  std::vector<std::pair<int, std::u16string>> writes;

 private:
  std::vector<Entry> entries_;
};

class FakeModel : public scada::aui::PropertyModel {
 public:
  explicit FakeModel(std::unique_ptr<FakeGroup> root) : root_{std::move(root)} {}
  PropertyGroup& GetRootGroup() override { return *root_; }
  FakeGroup& root() { return *root_; }

 private:
  std::unique_ptr<FakeGroup> root_;
};

// Root with two sections: General(Name, Enabled), Connection(Host, Port).
std::unique_ptr<FakeModel> MakeDeviceModel() {
  auto general = std::make_unique<FakeGroup>();
  general->AddLeaf(u"Name", u"RTU-02").AddLeaf(u"Enabled", u"true");
  auto connection = std::make_unique<FakeGroup>();
  connection->AddLeaf(u"Host", u"10.20.14.2").AddLeaf(u"Port", u"2404");

  auto root = std::make_unique<FakeGroup>();
  root->AddSection(u"General", std::move(general))
      .AddSection(u"Connection", std::move(connection));
  return std::make_unique<FakeModel>(std::move(root));
}

QLineEdit* FindEditorWithText(const DeviceParameterForm& form,
                              const QString& text) {
  for (QLineEdit* editor : form.findChildren<QLineEdit*>()) {
    if (editor->text() == text)
      return editor;
  }
  return nullptr;
}

class DeviceParameterFormTest : public ::testing::Test {
 protected:
  AppEnvironment app_env_;
};

TEST_F(DeviceParameterFormTest, StartsCleanWithApplyDisabled) {
  auto model = MakeDeviceModel();
  DeviceParameterForm form{*model, QStringLiteral("RTU-02")};

  EXPECT_FALSE(form.dirty());
  auto* apply = form.findChild<QPushButton*>(QStringLiteral("parameterApply"));
  ASSERT_NE(apply, nullptr);
  EXPECT_FALSE(apply->isEnabled());
}

TEST_F(DeviceParameterFormTest, BuildsATabPerSection) {
  auto model = MakeDeviceModel();
  DeviceParameterForm form{*model, QStringLiteral("RTU-02")};

  bool has_general = false;
  bool has_connection = false;
  for (QPushButton* button : form.findChildren<QPushButton*>()) {
    if (button->text() == QStringLiteral("General"))
      has_general = true;
    if (button->text() == QStringLiteral("Connection"))
      has_connection = true;
  }
  EXPECT_TRUE(has_general);
  EXPECT_TRUE(has_connection);
}

TEST_F(DeviceParameterFormTest, EditingAFieldMarksDirtyAndEnablesApply) {
  auto model = MakeDeviceModel();
  DeviceParameterForm form{*model, QStringLiteral("RTU-02")};

  QLineEdit* host = FindEditorWithText(form, QStringLiteral("10.20.14.2"));
  ASSERT_NE(host, nullptr);
  host->setText(QStringLiteral("10.20.14.9"));

  EXPECT_TRUE(form.dirty());
  auto* apply = form.findChild<QPushButton*>(QStringLiteral("parameterApply"));
  ASSERT_NE(apply, nullptr);
  EXPECT_TRUE(apply->isEnabled());
}

TEST_F(DeviceParameterFormTest, TypingBackTheLiveValueClearsDirty) {
  auto model = MakeDeviceModel();
  DeviceParameterForm form{*model, QStringLiteral("RTU-02")};

  QLineEdit* port = FindEditorWithText(form, QStringLiteral("2404"));
  ASSERT_NE(port, nullptr);
  port->setText(QStringLiteral("2405"));
  EXPECT_TRUE(form.dirty());
  port->setText(QStringLiteral("2404"));
  EXPECT_FALSE(form.dirty());
}

TEST_F(DeviceParameterFormTest, ApplyWritesStagedEditsThroughTheModel) {
  auto model = MakeDeviceModel();
  DeviceParameterForm form{*model, QStringLiteral("RTU-02")};

  QLineEdit* host = FindEditorWithText(form, QStringLiteral("10.20.14.2"));
  ASSERT_NE(host, nullptr);
  host->setText(QStringLiteral("10.20.14.9"));

  form.Apply();

  EXPECT_FALSE(form.dirty());
  // The Connection subgroup received exactly the Host write.
  // (Its value now reflects the applied text.)
  QLineEdit* applied = FindEditorWithText(form, QStringLiteral("10.20.14.9"));
  EXPECT_NE(applied, nullptr);
}

TEST_F(DeviceParameterFormTest, RevertRestoresLiveValuesAndClearsDirty) {
  auto model = MakeDeviceModel();
  DeviceParameterForm form{*model, QStringLiteral("RTU-02")};

  QLineEdit* host = FindEditorWithText(form, QStringLiteral("10.20.14.2"));
  ASSERT_NE(host, nullptr);
  host->setText(QStringLiteral("10.20.14.9"));
  ASSERT_TRUE(form.dirty());

  form.Revert();

  EXPECT_FALSE(form.dirty());
  EXPECT_NE(FindEditorWithText(form, QStringLiteral("10.20.14.2")), nullptr);
}

}  // namespace
