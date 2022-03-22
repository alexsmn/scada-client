#include "components/configuration_export/import_data_report.h"

#include "base/strings/utf_string_conversions.h"
#include "components/configuration_export/import_data.h"
#include "common/node_state.h"
#include "model/node_id_util.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"

void PrintProps(NodeService& node_service,
                const scada::NodeProperties& props,
                u16ostream& report) {
  for (auto& v : props) {
    const auto& prop = node_service.GetNode(v.first);
    report << u"  " << ToString16(prop.display_name()) << u" = "
           << ToString16(v.second) << std::endl;
  }
}

void PrintRefs(NodeService& node_service,
               const std::vector<ImportData::Reference>& refs,
               u16ostream& report) {
  for (auto& r : refs) {
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
