#include "components/configuration_export/excel_configuration_commands.h"

#include "base/base_paths.h"
#include "base/csv_reader.h"
#include "base/csv_writer.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/strings/sys_string_conversions.h"
#include "base/win/win_util2.h"
#include "components/configuration_export/export_data_collector.h"
#include "components/configuration_export/import_export.h"
#include "model/data_items_node_ids.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "services/dialog_service.h"
#include "services/task_manager.h"

#include <algorithm>
#include <fstream>
#include <set>

namespace {

const base::char16 kImportTitle[] = L"Импорт";
const base::char16 kExportTitle[] = L"Экспорт";
const char kDefaultFileName[] = "configuration.csv";

}  // namespace

void PrintProps(NodeService& node_service,
                const scada::NodeProperties& props,
                std::wostream& report) {
  for (auto& v : props) {
    const auto& prop = node_service.GetNode(v.first);
    report << L"  " << ToString16(prop.display_name()) << L" = "
           << v.second.get_or(base::string16{L"(Ошибка)"}) << std::endl;
  }
}

void PrintRefs(NodeService& node_service,
               const std::vector<ImportData::Reference>& refs,
               std::wostream& report) {
  for (auto& r : refs) {
    auto target_name = r.add_target_id.is_null()
                           ? L"(Нет)"
                           : GetDisplayName(node_service, r.add_target_id);
    report << ToString16(GetDisplayName(node_service, r.reference_type_id))
           << L" = " << ToString16(target_name) << std::endl;
  }
}

void ShowImportReport(const ImportData& import_data,
                      NodeService& node_service) {
  std::wofstream report("report.txt");

  report
      << L"Пожалуйста, убедитесь в правильности производимых изменений. Если "
         L"перечисленные"
      << std::endl
      << L"изменения не соответствуют ожидаемым, ответьте Нет на вопрос, "
         L"который появится"
      << std::endl
      << L"после закрытия данного окна." << std::endl
      << std::endl
      << L"ВНИМАНИЕ: При некорректном использовании данная операция может "
         L"привести к"
      << std::endl
      << L"потере конфигурации." << std::endl
      << std::endl;

  for (auto& p : import_data.create_nodes) {
    auto type_definition = node_service.GetNode(p.type_id);
    report << L"Создать: " << ToString16(type_definition.display_name())
           << std::endl;
    if (!p.id.is_null())
      report << L"  Ид = " << base::SysNativeMBToWide(NodeIdToScadaString(p.id))
             << std::endl;
    report << L"  Родитель = "
           << base::SysNativeMBToWide(NodeIdToScadaString(p.parent_id))
           << std::endl;
    report << L"  Имя = " << ToString16(p.attrs.display_name) << std::endl;
    PrintProps(node_service, p.props, report);
    PrintRefs(node_service, p.refs, report);
  }

  for (auto& p : import_data.modify_nodes) {
    auto node = node_service.GetNode(p.id);
    report << L"Изменить: " << ToString16(node.display_name()) << std::endl;
    if (!p.attrs.browse_name.empty())
      report << L"  Имя = " << ToString16(p.attrs.browse_name) << std::endl;
    PrintProps(node_service, p.props, report);
    PrintRefs(node_service, p.refs, report);
  }

  for (auto& p : import_data.delete_nodes) {
    auto node = node_service.GetNode(p);
    report << L"Удалить: " << ToString16(node.display_name()) << std::endl;
  }

  base::FilePath system_path;
  base::PathService::Get(base::DIR_WINDOWS, &system_path);

  auto command_line = L"\"" + system_path.AsEndingWithSeparator().value() +
                      L"notepad.exe\" report.txt";

  STARTUPINFO startup_info = {sizeof(startup_info)};
  PROCESS_INFORMATION process_info = {};
  if (!CreateProcess(nullptr, const_cast<LPTSTR>(command_line.c_str()), nullptr,
                     nullptr, FALSE, 0, nullptr, nullptr, &startup_info,
                     &process_info))
    throw ResourceError{L"Не удалось запустить Блокнот"};
  ::WaitForSingleObject(process_info.hProcess, INFINITE);
  CloseHandle(process_info.hProcess);
  CloseHandle(process_info.hThread);
}

void ApplyImportData(const ImportData& import_data, TaskManager& task_manager) {
  for (auto& p : import_data.create_nodes) {
    task_manager.PostInsertTask(p.id, p.parent_id, p.type_id, p.attrs, p.props);
    for (auto& ref : p.refs) {
      assert(ref.delete_target_id.is_null());
      assert(!ref.add_target_id.is_null());
      task_manager.PostAddReference(ref.reference_type_id, p.id,
                                    ref.add_target_id);
    }
  }

  for (auto& p : import_data.modify_nodes) {
    if (!p.attrs.empty() || !p.props.empty())
      task_manager.PostUpdateTask(p.id, p.attrs, p.props);
    for (auto& ref : p.refs) {
      if (!ref.delete_target_id.is_null())
        task_manager.PostDeleteReference(ref.reference_type_id, p.id,
                                         ref.add_target_id);
      if (!ref.add_target_id.is_null())
        task_manager.PostAddReference(ref.reference_type_id, p.id,
                                      ref.add_target_id);
    }
  }

  for (auto& p : import_data.delete_nodes)
    task_manager.PostDeleteTask(p);
}

void ExportConfigurationToExcel(NodeService& node_service,
                                DialogService& dialog_service,
                                const std::filesystem::path& path) {
  try {
    std::ofstream stream{path};
    if (!stream)
      throw ResourceError{L"Не удалось открыть файл."};

    auto data = CollectExportData(node_service);

    CsvWriter writer{stream};
    ExportConfiguration(data, writer);

  } catch (const ResourceError& e) {
    dialog_service.RunMessageBox((e.message() + L".").c_str(), kExportTitle,
                                 MessageBoxMode::Error);
    return;

  } catch (const std::runtime_error&) {
    dialog_service.RunMessageBox(L"Ошибка при экспорте.", kExportTitle,
                                 MessageBoxMode::Error);
    return;
  }

  if (dialog_service.RunMessageBox(
          L"Экспорт завершен. Открыть файл сейчас?", kExportTitle,
          MessageBoxMode::QuestionYesNo) == MessageBoxResult::Yes)
    win_util::OpenWithAssociatedProgram(path);
}

void ExportConfigurationToExcel(NodeService& node_service,
                                DialogService& dialog_service) {
  auto path = dialog_service.SelectSaveFile({kExportTitle, kDefaultFileName});
  if (path.empty())
    return;

  ExportConfigurationToExcel(node_service, dialog_service, path);
}

void ImportConfigurationFromExcel(NodeService& node_service,
                                  TaskManager& task_manager,
                                  DialogService& dialog_service,
                                  const std::filesystem::path& path) {
  std::wifstream stream{path};
  if (!stream) {
    dialog_service.RunMessageBox(L"Не удалось открыть файл.", kImportTitle,
                                 MessageBoxMode::Error);
    return;
  }

  CsvReader reader{stream, kNodeIdTitle};
  ImportData import_data;
  try {
    import_data = ImportConfiguration(node_service, reader);

  } catch (const ResourceError& e) {
    auto message = base::StringPrintf(
        L"Ошибка при импорте строки %d, столбца %d: %ls.", reader.row_index(),
        reader.cell_index(), e.message().c_str());
    dialog_service.RunMessageBox(message.c_str(), kImportTitle,
                                 MessageBoxMode::Error);
    return;
  }

  if (import_data.IsEmpty()) {
    dialog_service.RunMessageBox(L"Изменений не найдено.", kImportTitle,
                                 MessageBoxMode::Info);
    return;
  }

  ShowImportReport(import_data, node_service);

  bool confirmed =
      dialog_service.RunMessageBox(L"Применить изменения?", kImportTitle,
                                   MessageBoxMode::QuestionYesNoDefaultNo) ==
      MessageBoxResult::Yes;

  if (confirmed)
    ApplyImportData(import_data, task_manager);
}

void ImportConfigurationFromExcel(NodeService& node_service,
                                  TaskManager& task_manager,
                                  DialogService& dialog_service) {
  auto path = dialog_service.SelectOpenFile(kImportTitle);
  if (path.empty())
    return;

  ImportConfigurationFromExcel(node_service, task_manager, dialog_service,
                               path);
}
