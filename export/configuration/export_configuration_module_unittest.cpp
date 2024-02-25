#include "export_configuration_module.h"

#include "address_space/test/test_scada_node_states.h"
#include "aui/dialog_service_mock.h"
#include "base/csv_writer.h"
#include "base/file_path_util.h"
#include "base/files/scoped_temp_dir.h"
#include "common/test/node_state_matcher.h"
#include "common_resources.h"
#include "controller/command_registry.h"
#include "core/main_command_context.h"
#include "diff_data.h"
#include "diff_report.h"
#include "export_data.h"
#include "export_data_writer.h"
#include "main_window/main_window_mock.h"
#include "model/data_items_node_ids.h"
#include "node_service/static/static_node_service.h"
#include "services/task_manager_mock.h"

#include <fstream>
#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

class ExportConfigurationModuleTest : public Test {
 public:
  virtual void SetUp() override;

 protected:
  std::filesystem::path WriteExportDataToTempFile(
      const ExportData& export_data) const;

  StaticNodeService node_service_;
  StrictMock<MockTaskManager> task_manager_;
  BasicCommandRegistry<MainCommandContext> main_commands_;

  StrictMock<MockMainWindow> main_window_;
  StrictMock<MockDialogService> dialog_service_;

  MainCommandContext main_command_context_{.main_window = main_window_,
                                           .dialog_service = dialog_service_};

  ExportConfigurationModule module_{{.node_service_ = node_service_,
                                     .task_manager_ = task_manager_,
                                     .main_commands_ = main_commands_}};

  base::ScopedTempDir temp_dir_;
};

void ExportConfigurationModuleTest::SetUp() {
  node_service_.AddAll(GetScadaNodeStates());

  ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
}

TEST_F(ExportConfigurationModuleTest, Construct_RegistersCommands) {
  EXPECT_TRUE(main_commands_.FindCommand(ID_IMPORT_CONFIGURATION_FROM_EXCEL));
  EXPECT_TRUE(main_commands_.FindCommand(ID_EXPORT_CONFIGURATION_TO_EXCEL));
}

TEST_F(ExportConfigurationModuleTest, ImportCommand) {
  ExportData export_data{
      .props = {{.prop_decl_id = data_items::id::DiscreteItemType_Inversion,
                 .display_name = u"Inversion"},
                {.prop_decl_id = data_items::id::AnalogItemType_DisplayFormat,
                 .display_name = u"Display Format"}},
      .nodes = {
          {.node_id = {1, NamespaceIndexes::TS},
           .parent_id = data_items::id::DataItems,
           .type_display_name = u"DiscreteItemType",
           .type_id = data_items::id::DiscreteItemType,
           .display_name = u"TS 1",
           .property_values = {{.prop_decl_id =
                                    data_items::id::DiscreteItemType_Inversion,
                                .value = true}}},
          {.node_id = {1, NamespaceIndexes::TIT},
           .parent_id = data_items::id::DataItems,
           .type_display_name = u"AnalogItemType",
           .type_id = data_items::id::AnalogItemType,
           .display_name = u"TIT 1",
           .property_values = {
               {.prop_decl_id = data_items::id::AnalogItemType_DisplayFormat,
                .value = "#####"}}}}};

  std::filesystem::path export_file_path =
      WriteExportDataToTempFile(export_data);

  auto* command =
      main_commands_.FindCommand(ID_IMPORT_CONFIGURATION_FROM_EXCEL);
  ASSERT_THAT(command, NotNull());

  EXPECT_CALL(dialog_service_, SelectOpenFile(/*title=*/_))
      .WillOnce(Return(make_resolved_promise(export_file_path)));

  EXPECT_CALL(dialog_service_,
              RunMessageBox(/*message=*/_, /*title=*/_,
                            MessageBoxMode::QuestionYesNoDefaultNo))
      .WillOnce(Return(make_resolved_promise(MessageBoxResult::Yes)));

  EXPECT_CALL(
      task_manager_,
      PostInsertTask(NodeStateIs(scada::NodeState{
          .node_id = {1, NamespaceIndexes::TS},
          .type_definition_id = data_items::id::DiscreteItemType,
          .parent_id = data_items::id::DataItems,
          .attributes = {.display_name = u"TS 1"},
          .properties = {{data_items::id::DiscreteItemType_Inversion, true}}})))
      .WillOnce(Return(
          make_resolved_promise(scada::NodeId{1, NamespaceIndexes::TS})));

  EXPECT_CALL(task_manager_,
              PostInsertTask(NodeStateIs(scada::NodeState{
                  .node_id = {1, NamespaceIndexes::TIT},
                  .type_definition_id = data_items::id::AnalogItemType,
                  .parent_id = data_items::id::DataItems,
                  .attributes = {.display_name = u"TIT 1"},
                  .properties = {{data_items::id::AnalogItemType_DisplayFormat,
                                  "#####"}}})))
      .WillOnce(Return(
          make_resolved_promise(scada::NodeId{1, NamespaceIndexes::TIT})));

  ScopedImportReportSuppressor import_report_suppressor;

  command->execute_handler(main_command_context_);
}

std::filesystem::path ExportConfigurationModuleTest::WriteExportDataToTempFile(
    const ExportData& export_data) const {
  std::filesystem::path export_file_path =
      AsFilesystemPath(temp_dir_.GetPath()) / "export_file.csv";

  std::ofstream stream{export_file_path};
  CsvWriter csv_writer{stream};
  WriteExportData(export_data, csv_writer);

  return export_file_path;
}
