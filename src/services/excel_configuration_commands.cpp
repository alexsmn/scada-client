#include "client/client_utils.h"

#include <algorithm>

#include "base/base_paths.h"
#include "base/path_service.h"
#include "client/base/excel.h"
#include "client/base/table_reader.h"
#include "client/base/table_writer.h"
#include "client/services/property_defs.h"
#include "client/services/task_manager.h"
#include "common/format.h"
#include "common/node_ref_service.h"
#include "common/node_ref_util.h"
#include "common/scada_node_ids.h"
#include "core/node_management_service.h"
#include "ui/base/dialogs/select_file_dialog.h"

namespace {

const wchar_t kFileOpenError[] = L"Не удалось открыть файл.";
const wchar_t kNodeLoadError[] = L"Не удалось загрузить конфигурацию.";

const char kNodeIdTitle[] = "Ид";

typedef std::vector<NodeRef> NodeRefs;

void GetTypePids(NodeRefService& node_service, const NodeRef& type, bool recursive, NodeRefs& component_declarations, NodeRefs& reference_types) {
  assert(scada::IsTypeDefinition(type.node_class().get()));

  if (recursive) {
    if (const auto& supertype = type.supertype())
      GetTypePids(node_service, supertype, true, component_declarations, reference_types);
  }

  for (auto& component_declaration : type.aggregates())
    component_declarations.emplace_back(component_declaration);

  for (auto& reference : type.references())
    reference_types.emplace_back(reference.reference_type);
}

std::string FormatReferenceCell(const std::string& title, const scada::NodeId& prop_type_id) {
  return base::StringPrintf("%s @%s", title.c_str(), prop_type_id.ToString().c_str());
}

scada::NodeId ParseReferenceCell(const base::StringPiece& s) {
  auto p = s.rfind('@');
  if (p == base::StringPiece::npos)
    return scada::NodeId();
  auto n = s.substr(p + 1);
  return scada::NodeId::FromString(n);
}

struct AddressSpaceSnapshot {
  NodeRef GetNode(const scada::NodeId& node_id) const {
    auto i = nodes.find(node_id);
    return i == nodes.end() ? NodeRef{} : i->second;
  }

  std::map<scada::NodeId, NodeRef> nodes;
};

void MakeAddressSpaceSnapshot(NodeRefService& service, const scada::NodeId& parent_id, const scada::NodeId& type_definition_id,
    const std::function<void(AddressSpaceSnapshot snapshot)>& callback) {
  BrowseNodesRecursive(service, parent_id, type_definition_id, [callback](std::vector<NodeRef> nodes) {
    AddressSpaceSnapshot snapshot;
    for (auto& node : nodes)
      snapshot.nodes.emplace(node.id(), node);
    callback(std::move(snapshot));
  });
}

void ScanDeleteNodes(const AddressSpaceSnapshot& address_space, const scada::NodeId& type_definition_id,
                     const std::set<scada::NodeId>& exclude_ids, std::vector<scada::NodeId>& deleted_ids) {
  for (auto& p : address_space.nodes) {
    if (IsInstanceOf(p.second, type_definition_id) && exclude_ids.find(p.first) == exclude_ids.end())
      deleted_ids.emplace_back(p.first);
  }
}

} // namespace

struct ExportConfigurationData : public std::enable_shared_from_this<ExportConfigurationData> {
  explicit ExportConfigurationData(NodeRefService& node_service);

  void Export(const base::FilePath& path);

  void WriteRecursive(const scada::NodeId& parent_id);

  void WriteHeader();
  void WriteNode(const scada::NodeId& parent_id, const NodeRef& node);

  NodeRefService& node_service_;
  TableWriter writer_;
  std::function<void(const base::string16& message)> error_handler_;

  std::vector<NodeRef> component_declarations_;
  std::vector<NodeRef> reference_types_;
};

ExportConfigurationData::ExportConfigurationData(NodeRefService& node_service)
    : node_service_{node_service} {
}

void ExportConfigurationData::Export(const base::FilePath& path) {
  if (!writer_.Init(base::FilePath(path.value()))) {
    error_handler_(kFileOpenError);
    return;
  }

  auto self = shared_from_this();
  RequestNodes(node_service_, {id::DiscreteItemType, id::AnalogItemType},
      [self, this](const std::vector<scada::Status>& statuses, const std::vector<NodeRef>& nodes) {
        assert(statuses.size() == 2);
        assert(nodes.size() == 2);
        if (!statuses[0] || !statuses[1]) {
          error_handler_(kNodeLoadError);
          return;
        }

        auto& discrete_item_type = nodes[0];
        auto& analog_item_type = nodes[1];

        GetTypePids(node_service_, discrete_item_type, true, component_declarations_, reference_types_);
        GetTypePids(node_service_, analog_item_type, true, component_declarations_, reference_types_);

        WriteRecursive(id::DataItems);
      });
}

void ExportConfigurationData::WriteRecursive(const scada::NodeId& parent_id) {
  auto self = shared_from_this();
  BrowseNodesRecursive(node_service_, parent_id, {},
      [self, this, parent_id](std::vector<NodeRef> nodes) {
        nodes.erase(std::remove_if(nodes.begin(), nodes.end(), [](const NodeRef& node) {
          return !IsInstanceOf(node, id::DataGroupType) &&
                 !IsInstanceOf(node, id::DataItemType);
        }), nodes.end());

        for (auto& node : nodes)
          WriteNode(parent_id, node);

        for (auto& node : nodes)
          WriteRecursive(node.id());
      });
}

void ExportConfigurationData::WriteHeader() {
  writer_.StartRow();
  writer_.WriteCell(kNodeIdTitle);
  writer_.WriteCell("Родитель");
  writer_.WriteCell("Тип");
  writer_.WriteCell("Имя");
  for (auto& component_declaration : component_declarations_)
    writer_.WriteCell(FormatReferenceCell(component_declaration.browse_name(), component_declaration.id()));
  for (auto& reference_type : reference_types_)
    writer_.WriteCell(FormatReferenceCell(reference_type.browse_name(), reference_type.id()));
}

void ExportConfigurationData::WriteNode(const scada::NodeId& parent_id, const NodeRef& node) {
//  error_handler_(base::SysNativeMBToWide(e.what()) + L".");

  writer_.StartRow();
  writer_.WriteCell(node.id().ToString());
  writer_.WriteCell(parent_id.ToString());
  const auto& type = node.type_definition();
  writer_.WriteCell(type ? FormatReferenceCell(type.browse_name(), type.id()) : std::string());
  writer_.WriteCell(node.browse_name());
  for (auto& component_declaration : component_declarations_) {
    const auto& value = node[component_declaration.id()].value();
    const auto& str = value.get_or(std::string{});
    writer_.WriteCell(str);
  }
  for (auto& reference_type : reference_types_) {
    const auto& target = node.target(reference_type.id());
    writer_.WriteCell(target ?
        FormatReferenceCell(target.browse_name(), target.id()) :
        std::string());
  }
}

struct ImportData {
  struct Reference {
    scada::NodeId reference_type_id;
    scada::NodeId delete_target_id;
    scada::NodeId add_target_id;
  };

  struct CreateNode {
    scada::NodeId id;
    scada::NodeId type_id;
    scada::NodeId parent_id;
    scada::NodeAttributes attrs;
    scada::NodeProperties props;
    std::vector<Reference> refs;
  };

  bool IsEmpty() const {
    return create_nodes.empty() && modify_nodes.empty() && delete_nodes.empty();
  }

  std::vector<CreateNode> create_nodes;
  std::vector<CreateNode> modify_nodes;
  std::vector<scada::NodeId> delete_nodes;
};

void ImportConfiguration(TableReader& reader, const AddressSpaceSnapshot& address_space, ImportData& import_data) {
  std::set<scada::NodeId> listed_nodes;

  std::string cell;

  if (!reader.NextRow())
    throw std::runtime_error("Нет строки заголовка");

  // Skip Id, Parent, Type, Name.
  for (int i = 0; i < 4; ++i) {
    if (!reader.NextCell(cell))
      throw std::runtime_error("Неверный формат имени столбца");
  }

  std::vector<scada::NodeId> prop_type_ids;
  while (reader.NextCell(cell)) {
    auto prop_type_id = ParseReferenceCell(cell);
    if (prop_type_id.is_null())
      throw std::runtime_error("Неверный формат имени столбца");
    prop_type_ids.emplace_back(prop_type_id);
  }

  while (reader.NextRow()) {
    // Id.
    if (!reader.NextCell(cell))
      throw std::runtime_error("Ошибка при чтении идентификатора");
    auto node_id = scada::NodeId::FromString(cell);

    auto node = address_space.GetNode(node_id);
    if (node)
      listed_nodes.emplace(node.id());

    // Parent.
    if (!reader.NextCell(cell))
      throw std::runtime_error("Ошибка при чтении родителя");
    auto parent_id = scada::NodeId::FromString(cell);
    if (parent_id.is_null())
      throw std::runtime_error("Группа '" + cell + "' не найдена");

    // Type.
    if (!reader.NextCell(cell))
      throw std::runtime_error("Ошибка при чтении типа");
    auto type_definition = address_space.GetNode(ParseReferenceCell(cell));
    if (!type_definition)
      throw std::runtime_error("Тип с именем '" + cell + "' не найден");

    scada::NodeAttributes attrs;
    if (!reader.NextCell(cell))
      throw std::runtime_error("Ошибка при чтении имени");
    if (!node || node.browse_name() != cell)
      attrs.set_browse_name(std::move(cell));

    // Props & refs.
    scada::NodeProperties props;
    std::vector<ImportData::Reference> refs;
    size_t pid_index = 0;
    while (reader.NextCell(cell)) {
      auto pid = prop_type_ids[pid_index++];
      if (auto property_declaration = type_definition.GetAggregateDeclaration(pid)) {
        scada::Variant new_value;
        if (!StringToValue(cell.c_str(), property_declaration.data_type().id(), new_value)) {
          auto prop_type = property_declaration.data_type();
          auto prop_type_name = prop_type ? prop_type.browse_name() : "(Неизвестный)";
          throw std::runtime_error("Невозможно распознать значение ячейки '" + cell + "' с типом '" + prop_type_name + "'");
        }

        if (node) {
          auto value = node[property_declaration.id()].value();
          if (value == new_value)
            continue;
        }

        props.emplace_back(pid, std::move(new_value));

      } else if (type_definition.target(pid)) {
        auto referenced_id = ParseReferenceCell(cell);
        auto old_ref_node = node ? node.target(pid) : nullptr;
        auto old_referenced_id = old_ref_node ? old_ref_node.id() : scada::NodeId{};
        if (old_referenced_id != referenced_id)
          refs.push_back({ pid, old_referenced_id, referenced_id });
      }
    }

    if (node) {
      if (!attrs.empty() || !props.empty() || !refs.empty())
        import_data.modify_nodes.push_back({ node_id, type_definition.id(), parent_id, std::move(attrs), std::move(props), std::move(refs) });
    } else {
      import_data.create_nodes.push_back({ node_id, type_definition.id(), parent_id, std::move(attrs), std::move(props), std::move(refs) });
    }
  }

  ScanDeleteNodes(address_space, id::DataItemType, listed_nodes, import_data.delete_nodes);
}

void PrintProps(const NodeRef& type_definition, const scada::NodeProperties& props, std::ostream& report) {
  for (auto& v : props) {
    const auto& prop = type_definition[v.first];
    report << "  " << prop.browse_name() << " = " << v.second.get_or(std::string{"(Ошибка)"}) << std::endl;
  }
}

void PrintRefs(const AddressSpaceSnapshot& snapshot, const std::vector<ImportData::Reference>& refs, std::ostream& report) {
  for (auto& r : refs) {
    auto target_name = r.add_target_id.is_null() ? "(Нет)" : snapshot.GetNode(r.add_target_id).browse_name();
    report << snapshot.GetNode(r.reference_type_id).browse_name() << " = "
           << target_name << std::endl;
  }
}

void ShowImportReport(const ImportData& import_data, const AddressSpaceSnapshot& address_space) {
  std::ofstream report("report.txt");

  report << "Пожалуйста, убедитесь в правильности производимых изменений. Если перечисленные" << std::endl
         << "изменения не соответствуют ожидаемым, ответьте Нет на вопрос, который появится" << std::endl
         << "после закрытия данного окна." << std::endl
         << std::endl
         << "ВНИМАНИЕ: При некорректном использовании данная операция может привести к" << std::endl
         << "          потере конфигурации." << std::endl
         << std::endl;

  for (auto& p : import_data.create_nodes) {
    auto type_definition = address_space.GetNode(p.type_id);
    auto type_name = type_definition ? type_definition.browse_name() : "(Ошибка)";
    report << "Создать: " << type_name << std::endl;
    if (p.id != scada::NodeId())
      report << "  Ид = " << p.id.ToString() << std::endl;
    report << "  Родитель = " << p.parent_id.ToString() << std::endl;
    report << "  Имя = " << p.attrs.browse_name() << std::endl;
    if (type_definition)
      PrintProps(type_definition, p.props, report);
    PrintRefs(address_space, p.refs, report);
  }

  for (auto& p : import_data.modify_nodes) {
    auto node = address_space.GetNode(p.id);
    auto node_name = node.display_name();
    report << "Изменить: " << node_name << std::endl;
    auto type_definition = node ? node.type_definition() : nullptr;
    if (p.attrs.has(OpcUa_Attributes_BrowseName))
      report << "  Имя = " << p.attrs.browse_name() << std::endl;
    if (type_definition)
      PrintProps(type_definition, p.props, report);
    PrintRefs(address_space, p.refs, report);
  }

  for (auto& p : import_data.delete_nodes) {
    auto node = address_space.GetNode(p);
    auto node_name = node.display_name();
    report << "Удалить: " << node_name << std::endl;
  }

  base::FilePath system_path;
  PathService::Get(base::DIR_WINDOWS, &system_path);

  auto command_line = L"\"" + system_path.AsEndingWithSeparator().value() + L"notepad.exe\" report.txt";
  
  STARTUPINFO startup_info = { sizeof(startup_info) };
  PROCESS_INFORMATION process_info = {};
  if (!CreateProcess(nullptr, const_cast<LPTSTR>(command_line.c_str()), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &startup_info, &process_info))
    throw std::runtime_error("Не удалось запустить Блокнот");
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
      task_manager.PostAddReference(ref.reference_type_id, p.id, ref.add_target_id);
    }
  }

  for (auto& p : import_data.modify_nodes) {
    if (!p.attrs.empty() || !p.props.empty())
      task_manager.PostUpdateTask(p.id, p.attrs, p.props);
    for (auto& ref : p.refs) {
      if (!ref.delete_target_id.is_null())
        task_manager.PostDeleteReference(ref.reference_type_id, p.id, ref.add_target_id);
      if (!ref.add_target_id.is_null())
        task_manager.PostAddReference(ref.reference_type_id, p.id, ref.add_target_id);
    }
  }

  for (auto& p : import_data.delete_nodes)
    task_manager.PostDeleteTask(p);
}

void ExportConfigurationToExcel(DialogService& dialog_service, NodeRefService& node_service, const base::FilePath& path) {
  auto data = std::make_shared<ExportConfigurationData>(node_service);

  data->error_handler_ = [&dialog_service](const base::string16& message) {
    ShowMessageBox(dialog_service, message.c_str(), L"Экспорт", MB_ICONSTOP);
  };

  data->Export(path);
}

void ExportConfigurationToExcel(DialogService& dialog_service, NodeRefService& node_service) {
  class SelectFile : public ui::SelectFileDialog::Listener {
   public:
    explicit SelectFile(DialogService& dialog_service, NodeRefService& node_service)
        : node_service_(node_service),
          dialog_service_(dialog_service) {}

    virtual void FileSelected(const base::FilePath& path,
                              int index, void* params) override {
      ExportConfigurationToExcel(dialog_service_, node_service_, path);
    }

   private:
    DialogService& dialog_service_;
    NodeRefService& node_service_;
  };

  // TODO: Fixit.
  static SelectFile select_file(dialog_service, node_service);
  ui::SelectFileDialog::Create(&select_file, nullptr)->SelectFile(ui::SelectFileDialog::SELECT_SAVEAS_FILE, L"Экспорт",
      base::FilePath(L"configuration.csv"), nullptr, -1, base::string16(), nullptr, nullptr);
}

void ImportConfigurationFromExcel(DialogService& dialog_service, const base::FilePath& path, NodeRefService& node_service,
    TaskManager& task_manager) {
  struct Importer {
    Importer(DialogService& dialog_service, TaskManager& task_manager) : dialog_service{dialog_service}, task_manager{task_manager} {}

    DialogService& dialog_service;
    TaskManager& task_manager;
    TableReader reader;
    ImportData import_data;
  };

  auto importer = std::make_shared<Importer>(dialog_service, task_manager);
  if (!importer->reader.Init(path, kNodeIdTitle)) {
    ShowMessageBox(dialog_service, L"Не удалось открыть файл.", L"Импорт", MB_ICONSTOP);
    return;
  }

  MakeAddressSpaceSnapshot(node_service, OpcUaId_RootFolder, {}, [importer](AddressSpaceSnapshot address_space) {
    try {
      ImportConfiguration(importer->reader, address_space, importer->import_data);

    } catch (const std::exception& e) {
      auto message = base::StringPrintf("Ошибка при импорте строки %d, столбца %d: %s.",
          importer->reader.row_index(), importer->reader.cell_index(), e.what());
      ShowMessageBox(importer->dialog_service, base::SysNativeMBToWide(message).c_str(), L"Импорт", MB_ICONSTOP);
      return;
    }

    if (importer->import_data.IsEmpty()) {
      ShowMessageBox(importer->dialog_service, L"Изменений не найдено.", L"Импорт", MB_ICONINFORMATION);
      return;
    }

    ShowImportReport(importer->import_data, address_space);

    if (ShowMessageBox(importer->dialog_service, L"Применить изменения?", L"Импорт", MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) != IDYES)
      return;

    ApplyImportData(importer->import_data, importer->task_manager);
  });
}

void ImportConfigurationFromExcel(DialogService& dialog_service, NodeRefService& node_service, TaskManager& task_manager) {
  class SelectFile : public ui::SelectFileDialog::Listener {
   public:
    SelectFile(DialogService& dialog_service, NodeRefService& node_service, TaskManager& task_manager)
        : dialog_service_(dialog_service),
          node_service_(node_service),
          task_manager_(task_manager) {}

    virtual void FileSelected(const base::FilePath& path,
                              int index, void* params) override {
      ImportConfigurationFromExcel(dialog_service_, path, node_service_, task_manager_);
    }

   private:
    DialogService& dialog_service_;
    NodeRefService& node_service_;
    TaskManager& task_manager_;
  };

  // TODO: Fixit.
  static SelectFile select_file(dialog_service, node_service, task_manager);
  ui::SelectFileDialog::Create(&select_file, nullptr)->
      SelectFile(ui::SelectFileDialog::SELECT_OPEN_FILE, L"Импорт", base::FilePath(), nullptr, -1, base::string16(), nullptr, nullptr);
}