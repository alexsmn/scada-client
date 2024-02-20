#define _SILENCE_CXX20_CODECVT_FACETS_DEPRECATION_WARNING

#include "export/configuration/import_data_report.h"

#include "aui/resource_error.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/win_util2.h"
#include "common/node_state.h"
#include "export/configuration/import_data.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

#include <fstream>

using u16ostream = std::basic_ostream<char16_t>;

bool s_import_report_enabled = true;

ScopedImportReportSuppressor::ScopedImportReportSuppressor() {
  s_import_report_enabled = false;
}

ScopedImportReportSuppressor ::~ScopedImportReportSuppressor() {
  s_import_report_enabled = true;
}

void PrintProps(NodeService& node_service,
                const scada::NodeProperties& props,
                u16ostream& report) {
  for (const auto& v : props) {
    const auto& prop = node_service.GetNode(v.first);
    report << u"  " << ToString16(prop.display_name()) << u" = "
           << ToString16(v.second) << std::endl;
  }
}

void PrintRefs(NodeService& node_service,
               const std::vector<ImportData::Reference>& refs,
               u16ostream& report) {
  for (const auto& r : refs) {
    auto target_name = r.add_target_id.is_null()
                           ? u"(Нет)"
                           : GetDisplayName(node_service, r.add_target_id);
    report << ToString16(GetDisplayName(node_service, r.reference_type_id))
           << u" = " << ToString16(target_name) << std::endl;
  }
}

void PrintImportReport(u16ostream& report,
                       const ImportData& import_data,
                       NodeService& node_service) {
  report
      << u"Пожалуйста, убедитесь в правильности производимых изменений. Если "
         u"перечисленные"
      << std::endl
      << u"изменения не соответствуют ожидаемым, ответьте Нет на вопрос, "
         u"который появится"
      << std::endl
      << u"после закрытия данного окна." << std::endl
      << std::endl
      << u"ВНИМАНИЕ: При некорректном использовании данная операция может "
         u"привести к"
      << std::endl
      << u"потере конфигурации." << std::endl
      << std::endl;

  for (auto& p : import_data.create_nodes) {
    auto type_definition = node_service.GetNode(p.type_id);
    report << u"Создать: " << ToString16(type_definition.display_name())
           << std::endl;
    if (!p.id.is_null())
      report << u"  Ид = " << base::UTF8ToUTF16(NodeIdToScadaString(p.id))
             << std::endl;
    report << u"  Родитель = "
           << base::UTF8ToUTF16(NodeIdToScadaString(p.parent_id)) << std::endl;
    report << u"  Имя = " << ToString16(p.attrs.display_name) << std::endl;
    PrintProps(node_service, p.props, report);
    PrintRefs(node_service, p.refs, report);
  }

  for (auto& p : import_data.modify_nodes) {
    auto node = node_service.GetNode(p.id);
    report << u"Изменить: " << ToString16(node.display_name()) << std::endl;
    if (!p.attrs.browse_name.empty())
      report << u"  Имя = " << ToString16(p.attrs.browse_name) << std::endl;
    PrintProps(node_service, p.props, report);
    PrintRefs(node_service, p.refs, report);
  }

  for (auto& p : import_data.delete_nodes) {
    auto node = node_service.GetNode(p);
    report << u"Удалить: " << ToString16(node.display_name()) << std::endl;
  }
}

void ShowImportReport(const ImportData& import_data,
                      NodeService& node_service) {
  if (!s_import_report_enabled) {
    return;
  }

  std::basic_ofstream<char16_t> report("report.txt");
  PrintImportReport(report, import_data, node_service);

  base::FilePath system_path;
  base::PathService::Get(base::DIR_WINDOWS, &system_path);

  auto command_line = L"\"" + system_path.AsEndingWithSeparator().value() +
                      L"notepad.exe\" report.txt";

  STARTUPINFO startup_info = {sizeof(startup_info)};
  PROCESS_INFORMATION process_info = {};
  if (!CreateProcess(nullptr, const_cast<LPTSTR>(command_line.c_str()), nullptr,
                     nullptr, FALSE, 0, nullptr, nullptr, &startup_info,
                     &process_info)) {
    throw ResourceError{u"Не удалось открыть блокнот"};
  }
  ::WaitForSingleObject(process_info.hProcess, INFINITE);
  CloseHandle(process_info.hProcess);
  CloseHandle(process_info.hThread);
}
