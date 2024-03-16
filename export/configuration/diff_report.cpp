#define _SILENCE_CXX20_CODECVT_FACETS_DEPRECATION_WARNING

#include "export/configuration/diff_report.h"

#include "aui/resource_error.h"
#include "base/file_path_util.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/scoped_process_information.h"
#include "base/win/win_util2.h"
#include "common/node_state.h"
#include "export/configuration/diff_data.h"
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
                std::span<const scada::NodeProperty> props,
                u16ostream& report) {
  for (const auto& [prop_decl_id, value] : props) {
    const auto& prop = node_service.GetNode(prop_decl_id);
    report << u"  " << ToString16(prop.display_name()) << u" = "
           << ToString16(value) << std::endl;
  }
}

void PrintRefs(NodeService& node_service,
               std::span<const DiffData::Reference> refs,
               u16ostream& report) {
  for (const auto& r : refs) {
    auto target_name = r.add_target_id.is_null()
                           ? u"(Нет)"
                           : GetDisplayName(node_service, r.add_target_id);
    report << ToString16(GetDisplayName(node_service, r.reference_type_id))
           << u" = " << ToString16(target_name) << std::endl;
  }
}

void PrintRefs(NodeService& node_service,
               std::span<const scada::ReferenceDescription> refs,
               u16ostream& report) {
  for (const auto& r : refs) {
    assert(!r.reference_type_id.is_null());
    assert(!r.node_id.is_null());
    auto ref_name = GetDisplayName(node_service, r.reference_type_id);
    auto target_name = GetDisplayName(node_service, r.node_id);
    report << ToString16(ref_name) << u" = " << ToString16(target_name)
           << std::endl;
  }
}

const char16_t kDiffReportHeader[] =
    uR"(Пожалуйста, убедитесь в правильности производимых изменений. Если перечисленные
изменения не соответствуют ожидаемым, ответьте Нет на вопрос, который появится
после закрытия данного окна.

ВНИМАНИЕ: При некорректном использовании данная операция может привести к
потере конфигурации.)";

void PrintDiffReport(u16ostream& report,
                     const DiffData& diff,
                     NodeService& node_service) {
  report << kDiffReportHeader << std::endl << std::endl;

  for (auto& node_state : diff.create_nodes) {
    auto type_definition = node_service.GetNode(node_state.type_definition_id);
    report << u"Создать: " << ToString16(type_definition.display_name())
           << std::endl;
    if (!node_state.node_id.is_null()) {
      report << u"  Ид = "
             << base::UTF8ToUTF16(NodeIdToScadaString(node_state.node_id))
             << std::endl;
    }
    report << u"  Родитель = "
           << base::UTF8ToUTF16(NodeIdToScadaString(node_state.parent_id))
           << std::endl;
    report << u"  Имя = " << ToString16(node_state.attributes.display_name)
           << std::endl;
    PrintProps(node_service, node_state.properties, report);
    PrintRefs(node_service, node_state.references, report);
  }

  for (auto& p : diff.modify_nodes) {
    auto node = node_service.GetNode(p.id);
    report << u"Изменить: " << ToString16(node.display_name()) << std::endl;
    if (!p.attrs.browse_name.empty())
      report << u"  Имя = " << ToString16(p.attrs.browse_name) << std::endl;
    PrintProps(node_service, p.props, report);
    PrintRefs(node_service, p.refs, report);
  }

  for (auto& p : diff.delete_nodes) {
    auto node = node_service.GetNode(p);
    report << u"Удалить: " << ToString16(node.display_name()) << std::endl;
  }
}

void OpenNotepad(const std::filesystem::path& path) {
  base::FilePath system_path;
  if (!base::PathService::Get(base::DIR_WINDOWS, &system_path)) {
    return;
  }

  std::wstring command_line =
      std::format(L"\"{}notepad.exe\" {}",
                  system_path.AsEndingWithSeparator().value(), path.wstring());

  STARTUPINFO startup_info = {sizeof(startup_info)};
  PROCESS_INFORMATION raw_process_info = {};

  if (!CreateProcess(/*app_name=*/nullptr,
                     const_cast<LPTSTR>(command_line.c_str()),
                     /*proc_attrs=*/nullptr,
                     /*thread_attrs=*/nullptr, /*inherit_handles=*/FALSE,
                     /*create_flags=*/0, /*env=*/nullptr, /*cur_dir=*/nullptr,
                     &startup_info, &raw_process_info)) {
    throw ResourceError{u"Не удалось открыть блокнот"};
  }

  base::win::ScopedProcessInformation proc_info{raw_process_info};
  ::WaitForSingleObject(proc_info.process_handle(), INFINITE);
}

void ShowDiffReport(const DiffData& diff, NodeService& node_service) {
  if (!s_import_report_enabled) {
    return;
  }

  base::FilePath temp_dir;
  if (!base::GetTempDir(&temp_dir)) {
    return;
  }

  auto report_path = AsFilesystemPath(temp_dir.AppendASCII("report.txt"));

  std::basic_ofstream<char16_t> report{report_path};
  PrintDiffReport(report, diff, node_service);

  OpenNotepad(report_path);
}
