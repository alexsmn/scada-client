#include "components/configuration_export/import_data_report.h"

#include "base/strings/sys_string_conversions.h"
#include "components/configuration_export/import_data.h"
#include "core/configuration_types.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

void PrintProps(NodeService& node_service,
                const scada::NodeProperties& props,
                std::wostream& report) {
  for (auto& v : props) {
    const auto& prop = node_service.GetNode(v.first);
    report << L"  " << ToString16(prop.display_name()) << L" = "
           << v.second.get_or(std::wstring{L"(Ошибка)"}) << std::endl;
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

void PrintImportReport(std::wostream& report,
                       const ImportData& import_data,
                       NodeService& node_service) {
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
}
