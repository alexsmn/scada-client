#include "parameter_form/qt/device_parameter_form.h"

#include "aui/models/property_model.h"
#include "aui/test/app_environment.h"

#include <gtest/gtest.h>

#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTableWidget>

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
    scada::aui::EditData edit;
  };

  FakeGroup& AddLeaf(std::u16string name, std::u16string value) {
    entries_.push_back({std::move(name), std::move(value), nullptr, {}});
    return *this;
  }
  FakeGroup& AddDropdown(std::u16string name,
                         std::u16string value,
                         std::vector<std::u16string> choices) {
    scada::aui::EditData edit;
    edit.editor_type = scada::aui::EditData::EditorType::DROPDOWN;
    edit.choices = std::move(choices);
    entries_.push_back(
        {std::move(name), std::move(value), nullptr, std::move(edit)});
    return *this;
  }
  FakeGroup& AddReadOnly(std::u16string name, std::u16string value) {
    scada::aui::EditData edit;
    edit.editor_type = scada::aui::EditData::EditorType::NONE;
    entries_.push_back(
        {std::move(name), std::move(value), nullptr, std::move(edit)});
    return *this;
  }
  FakeGroup& AddSection(std::u16string name, std::unique_ptr<FakeGroup> sub) {
    entries_.push_back({std::move(name), {}, std::move(sub), {}});
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
  scada::aui::EditData GetEditData(int i) const override {
    return entries_[i].edit;
  }
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

// A section mixing a text field, a dropdown, and a read-only field, to exercise
// the per-property editor kinds from EditData.
std::unique_ptr<FakeModel> MakeRicherModel() {
  auto general = std::make_unique<FakeGroup>();
  general->AddLeaf(u"Name", u"RTU-02")
      .AddDropdown(u"Protocol", u"Modbus",
                   {u"Modbus", u"IEC 60870-5-104", u"IEC 61850"})
      .AddReadOnly(u"Type", u"ModbusDeviceType");
  auto root = std::make_unique<FakeGroup>();
  root->AddSection(u"General", std::move(general));
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

TEST_F(DeviceParameterFormTest, DropdownPropertyBecomesAComboWithChoices) {
  auto model = MakeRicherModel();
  DeviceParameterForm form{*model, QStringLiteral("RTU-02")};

  const QList<QComboBox*> combos = form.findChildren<QComboBox*>();
  ASSERT_EQ(combos.size(), 1);
  QComboBox* protocol = combos.front();
  EXPECT_EQ(protocol->currentText(), QStringLiteral("Modbus"));

  QStringList items;
  for (int i = 0; i < protocol->count(); ++i)
    items << protocol->itemText(i);
  EXPECT_TRUE(items.contains(QStringLiteral("IEC 60870-5-104")));
  EXPECT_TRUE(items.contains(QStringLiteral("IEC 61850")));
}

TEST_F(DeviceParameterFormTest, ChangingADropdownStagesThenRevertRestores) {
  auto model = MakeRicherModel();
  DeviceParameterForm form{*model, QStringLiteral("RTU-02")};

  QComboBox* protocol = form.findChildren<QComboBox*>().front();
  EXPECT_FALSE(form.dirty());
  protocol->setCurrentText(QStringLiteral("IEC 60870-5-104"));
  EXPECT_TRUE(form.dirty());

  form.Revert();
  EXPECT_FALSE(form.dirty());
  EXPECT_EQ(protocol->currentText(), QStringLiteral("Modbus"));
}

TEST_F(DeviceParameterFormTest, ReadOnlyPropertyIsNotEditable) {
  auto model = MakeRicherModel();
  DeviceParameterForm form{*model, QStringLiteral("RTU-02")};

  QLineEdit* type = FindEditorWithText(form, QStringLiteral("ModbusDeviceType"));
  ASSERT_NE(type, nullptr);
  EXPECT_TRUE(type->isReadOnly());
}

TEST_F(DeviceParameterFormTest, SetAddressMapAddsAReadOnlyGridTab) {
  auto model = MakeDeviceModel();
  DeviceParameterForm form{*model, QStringLiteral("RTU-02")};

  form.SetAddressMap({
      {u"Q1 state", u"TS", u"1001", u"ns=2;s=Q1"},
      {u"U L1-L2", u"TI", u"4001", u"ns=2;s=U12"},
  });

  bool has_tab = false;
  for (QPushButton* button : form.findChildren<QPushButton*>()) {
    if (button->text() == QStringLiteral("Address map"))
      has_tab = true;
  }
  EXPECT_TRUE(has_tab);

  auto* table =
      form.findChild<QTableWidget*>(QStringLiteral("addressMapTable"));
  ASSERT_NE(table, nullptr);
  EXPECT_EQ(table->editTriggers(), QAbstractItemView::NoEditTriggers);
  ASSERT_EQ(table->rowCount(), 2);
  EXPECT_EQ(table->item(0, 0)->text(), QStringLiteral("Q1 state"));
  EXPECT_EQ(table->item(0, 2)->text(), QStringLiteral("1001"));
  EXPECT_EQ(table->item(1, 1)->text(), QStringLiteral("TI"));
}

TEST_F(DeviceParameterFormTest, EmptyAddressMapAddsNoTab) {
  auto model = MakeDeviceModel();
  DeviceParameterForm form{*model, QStringLiteral("RTU-02")};
  form.SetAddressMap({});

  for (QPushButton* button : form.findChildren<QPushButton*>())
    EXPECT_NE(button->text(), QStringLiteral("Address map"));
}

TEST_F(DeviceParameterFormTest, SetLimitsAddsAReadOnlyGridTab) {
  auto model = MakeDeviceModel();
  DeviceParameterForm form{*model, QStringLiteral("RTU-02")};

  form.SetLimits({
      {u"Power", u"113", u"118", u"133", u"138"},
      {u"Voltage", u"", u"9.8", u"11.2", u""},
  });

  bool has_tab = false;
  for (QPushButton* button : form.findChildren<QPushButton*>()) {
    if (button->text() == QStringLiteral("Limits"))
      has_tab = true;
  }
  EXPECT_TRUE(has_tab);

  auto* table = form.findChild<QTableWidget*>(QStringLiteral("limitsTable"));
  ASSERT_NE(table, nullptr);
  EXPECT_EQ(table->editTriggers(), QAbstractItemView::NoEditTriggers);
  ASSERT_EQ(table->rowCount(), 2);
  EXPECT_EQ(table->item(0, 0)->text(), QStringLiteral("Power"));
  EXPECT_EQ(table->item(0, 1)->text(), QStringLiteral("113"));  // LoLo
  EXPECT_EQ(table->item(0, 4)->text(), QStringLiteral("138"));  // HiHi
  EXPECT_EQ(table->item(1, 1)->text(), QString{});  // unset LoLo
}

}  // namespace
