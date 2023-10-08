#include "components/main/selection_commands.h"

#include "base/command_line.h"
#include "base/files/file_util.h"
#include "base/promise_executor.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/clipboard.h"
#include "client_utils.h"
#include "clipboard/clipboard_util.h"
#include "common_resources.h"
#include "components/change_password/change_password_dialog.h"
#include "components/device_metrics/device_metrics_command.h"
#include "components/events/events_component.h"
#include "components/filesystem/filesystem_commands.h"
#include "components/limits/limit_dialog.h"
#include "components/main/main_window_manager.h"
#include "components/main/main_window_util.h"
#include "components/main/opened_view.h"
#include "components/node_properties/node_property_component.h"
#include "components/node_table/node_table_component.h"
#include "components/summary/summary_component.h"
#include "components/table/table_component.h"
#include "components/timed_data/timed_data_component.h"
#include "components/transmission/transmission_component.h"
#include "components/watch/watch_component.h"
#include "components/write/write_dialog.h"
#include "controller/window_definition_util.h"
#include "events/node_event_provider.h"
#include "main_window.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/filesystem_node_ids.h"
#include "model/node_id_util.h"
#include "model/scada_node_ids.h"
#include "model/security_node_ids.h"
#include "node_service/node_service.h"
#include "node_service/node_util.h"
#include "scada/node_management_service.h"
#include "scada/session_service.h"
#include "controller/selection_model.h"
#include "services/create_tree.h"
#include "services/dialog_service.h"
#include "services/file_cache.h"
#include "services/task_manager.h"
#include "window_definition_builder.h"
#include "controller/window_info.h"

#if !defined(UI_WT)
#include "components/graph/graph_component.h"
#include "components/modus/modus_component.h"
#endif

namespace {

bool CanCreateSomething(const NodeRef& node) {
  if (node.target(scada::id::Creates))
    return true;

  for (auto type = node.type_definition(); type; type = type.supertype()) {
    if (type.target(scada::id::Creates))
      return true;
  }

  return false;
}

#if defined(UI_QT)
std::string EncodeUri(std::string_view str) {
  return QByteArray{str.data(), static_cast<int>(str.size())}
      .toPercentEncoding()
      .toStdString();
}
#else
std::string EncodeUri(std::string_view str) {
  return std::string{str};
}
#endif

void PrintReferences(std::ostream& stream,
                     base::span<const NodeRef::Reference> references) {
  for (const auto& r : references) {
    stream << "{";
    stream << "forward: " << r.forward;
    stream << ", ";
    stream << "type: " << r.reference_type.browse_name();
    stream << ", ";
    stream << "target: " << NodeIdToScadaString(r.target.node_id());
    stream << "}";
    stream << std::endl;
  }
}

std::string GetNodeDebugInfo(const NodeRef& node) {
  std::ostringstream stream;
  stream << NodeIdToScadaString(node.node_id()) << std::endl;

  stream << "Fetched: " << node.fetched() << std::endl;
  stream << "Children fetched: " << node.children_fetched() << std::endl;
  stream << std::endl;

  stream << "Attributes:" << std::endl;
  stream << "BrowseName: " << node.browse_name() << std::endl;
  stream << "DisplayName: " << node.display_name() << std::endl;
  stream << "TypeDefinition: " << node.type_definition().browse_name()
         << std::endl;
  stream << "Supertype: " << node.supertype().browse_name() << std::endl;
  stream << std::endl;

  stream << "References:" << std::endl;
  PrintReferences(stream, node.references());
  stream << std::endl;

  stream << "Inverse references:" << std::endl;
  PrintReferences(stream, node.inverse_references());
  stream << std::endl;

  return stream.str();
}

// TODO: Revise ownership. Should be probably captured by a shared pointer.
void RegisterFileSystemCommands(SelectionCommands& selection_commands,
                                CommandRegistry& command_registry,
                                NodeService& node_service,
                                TaskManager& task_manager,
                                CreateTree& create_tree) {
  // |selection_| and |dialog_service_| are never null in command handlers.

  const auto& file_type = node_service.GetNode(filesystem::id::FileType);
  file_type.Fetch(NodeFetchStatus::NodeOnly());

  command_registry.AddCommand(
      Command{ID_ADD_FILE}
          .set_execute_handler([&selection_commands, &task_manager] {
            AddFile(selection_commands.selection()->node(),
                    *selection_commands.dialog_service(), task_manager);
          })
          .set_available_handler(
              [&create_tree, &selection_commands, file_type] {
                return create_tree.CanCreate(
                    selection_commands.selection()->node(), file_type);
              }));

  const auto& file_directory_type =
      node_service.GetNode(filesystem::id::FileType);
  file_directory_type.Fetch(NodeFetchStatus::NodeOnly());

  command_registry.AddCommand(
      Command{ID_CREATE_FILE_DIRECTORY}
          .set_execute_handler([&selection_commands, &task_manager] {
            CreateFileDirectory(selection_commands.selection()->node(),
                                *selection_commands.dialog_service(),
                                task_manager);
          })
          .set_available_handler([&create_tree, &selection_commands,
                                  file_directory_type] {
            return create_tree.CanCreate(selection_commands.selection()->node(),
                                         file_directory_type);
          }));
}

void RegisterDeviceEnableCommand(SelectionCommands& selection_commands,
                                 CommandRegistry& command_registry,
                                 const scada::SessionService& session_service,
                                 TaskManager& task_manager,
                                 unsigned command_id,
                                 bool enable) {
  command_registry.AddCommand(
      Command{command_id}
          .set_execute_handler([&selection_commands, &task_manager, enable] {
            ExecuteDisableItem(task_manager,
                               selection_commands.selection()->node(), !enable);
          })
          .set_enabled_handler([&selection_commands, enable] {
            return selection_commands.selection()
                       ->node()[devices::id::DeviceType_Disabled]
                       .value()
                       .get_or(false) == enable;
          })
          .set_available_handler([&selection_commands, &session_service] {
            return session_service.HasPrivilege(scada::Privilege::Configure) &&
                   selection_commands.selection()
                       ->node()[devices::id::DeviceType_Disabled];
          }));
}

void RegisterMethodCommand(SelectionCommands& selection_commands,
                           CommandRegistry& command_registry,
                           const scada::SessionService& session_service,
                           unsigned command_id,
                           const scada::NodeId& method_id) {
  command_registry.AddCommand(
      Command{command_id}
          .set_execute_handler([&selection_commands, method_id] {
            selection_commands.CallMethod(
                selection_commands.selection()->node(), method_id, {});
          })
          .set_available_handler([&selection_commands, &session_service] {
            return session_service.HasPrivilege(scada::Privilege::Control) &&
                   IsInstanceOf(selection_commands.selection()->node(),
                                data_items::id::DataItemType);
          }));
}

void RegisterOpenViewCommand(SelectionCommands& selection_commands,
                             CommandRegistry& command_registry,
                             unsigned command_id,
                             const WindowInfo& window_info) {
  command_registry.AddCommand(
      Command{command_id}
          .set_execute_handler([&selection_commands, &window_info] {
            ::OpenView(
                selection_commands.main_window(),
                selection_commands.GetOpenWindowDefinition(&window_info));
          })
          .set_available_handler([&selection_commands] {
            return !selection_commands.selection()->empty();
          }));
}

}  // namespace

// SelectionCommands

SelectionCommands::SelectionCommands(SelectionCommandsContext&& context)
    : SelectionCommandsContext{std::move(context)} {
  // |selection_| and |dialog_service_| are never null in command handlers.

  RegisterOpenViewCommand(*this, command_registry_, ID_OPEN_TABLE,
                          kTableWindowInfo);
#if !defined(UI_WT)
  RegisterOpenViewCommand(*this, command_registry_, ID_OPEN_GRAPH,
                          kGraphWindowInfo);
#endif
  RegisterOpenViewCommand(*this, command_registry_, ID_OPEN_SUMMARY,
                          kSummaryWindowInfo);

  command_registry_.AddCommand(
      Command{ID_OPEN_DEVICE_METRICS}
          .set_execute_handler([this] {
            MakeDeviceMetricsWindowDefinition(selection_->node())
                .then([weak_ptr = weak_ptr_factory_.GetWeakPtr()](
                          const WindowDefinition& window_definition) {
                  if (auto* ptr = weak_ptr.get())
                    ptr->OpenWindow(window_definition);
                });
          })
          .set_available_handler([this] {
            return IsInstanceOf(selection()->node(), devices::id::DeviceType);
          }));

  RegisterFileSystemCommands(*this, command_registry_, node_service_,
                             task_manager_, create_tree_);

  RegisterDeviceEnableCommand(*this, command_registry_, session_service_,
                              task_manager_, ID_ITEM_ENABLE, true);
  RegisterDeviceEnableCommand(*this, command_registry_, session_service_,
                              task_manager_, ID_ITEM_DISABLE, false);

  RegisterMethodCommand(*this, command_registry_, session_service_,
                        ID_DEV1_REFR, devices::id::DeviceType_Interrogate);
  RegisterMethodCommand(*this, command_registry_, session_service_,
                        ID_DEV1_SYNC, devices::id::DeviceType_SyncClock);

  command_registry_.AddCommand(
      Command{ID_OPEN_EVENTS}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       GetOpenWindowDefinition(&kEventJournalWindowInfo)
                           .then([](const WindowDefinition& window_def) {
                             auto new_window_def = window_def;
                             new_window_def.AddItem("Window").SetString(
                                 "mode", "Current");
                             return new_window_def;
                           }),
                       true);
          })
          .set_available_handler(
              [this] { return !selection_->empty() ? this : nullptr; }));

  command_registry_.AddCommand(
      Command{ID_HISTORICAL_EVENTS}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       GetOpenWindowDefinition(&kEventJournalWindowInfo), true);
          })
          .set_available_handler(
              [this] { return !selection_->empty() ? this : nullptr; }));

#if !defined(UI_WT)
  command_registry_.AddCommand(
      Command{ID_OPEN_DISPLAY}
          .set_execute_handler([this] { OpenModusView(selection_->node()); })
          .set_available_handler(
              [this] { return selection_->timed_data().connected(); }));
#endif

  command_registry_.AddCommand(
      Command{ID_TIMED_DATA_VIEW}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       GetOpenWindowDefinition(&kTimedDataWindowInfo));
          })
          .set_available_handler(
              [this] { return selection_->timed_data().connected(); }));

  command_registry_.AddCommand(
      Command{ID_EDIT_LIMITS}
          .set_execute_handler([this] {
            ShowLimitsDialog(*dialog_service_,
                             {selection_->node(), task_manager_});
          })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Control) &&
                   IsInstanceOf(selection_->node(),
                                data_items::id::AnalogItemType);
          }));

  command_registry_.AddCommand(
      Command{ID_OPEN_GROUP_TABLE}
          .set_execute_handler([this] {
            // TODO: Capture |main_window_| by weak pointer.
            MakeGroupWindowDefinition(&kTableWindowInfo, selection_->node())
                .then([main_window = main_window_](
                          const std::optional<WindowDefinition>& window_def) {
                  if (window_def.has_value())
                    ::OpenView(main_window, *window_def);
                });
          })
          .set_available_handler(
              [this] { return selection_->timed_data().connected(); }));

  command_registry_.AddCommand(
      Command{ID_ITEM_PARAMS}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       MakeSingleWindowDefinition(&kNodePropertyWindowInfo,
                                                  selection_->node()));
          })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   selection_->node();
          }));

  command_registry_.AddCommand(
      Command{ID_TABLE_CONFIG}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       MakeSingleWindowDefinition(&kTableEditorWindowInfo,
                                                  selection_->node()));
          })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   CanCreateSomething(selection_->node());
          }));

  command_registry_.AddCommand(
      Command{ID_OPEN_WATCH}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       MakeSingleWindowDefinition(&kWatchWindowInfo,
                                                  selection_->node()));
          })
          .set_available_handler([this] {
            return IsInstanceOf(selection_->node(), devices::id::DeviceType);
          }));

  command_registry_.AddCommand(
      Command{ID_CHANGE_PASSWORD}
          .set_execute_handler([this] {
            ShowChangePasswordDialog(
                *dialog_service_,
                ChangePasswordContext{selection_->node(), local_events_,
                                      profile_});
          })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   IsInstanceOf(selection_->node(), security::id::UserType);
          }));

  command_registry_.AddCommand(
      Command{ID_DUMP_DEBUG_INFO}
          .set_execute_handler([this] { DumpDebugInfo(); })
          .set_available_handler(
              [this] { return selection_->timed_data().connected(); }));

  // ID_TRANSMISSION_VIEW
  command_registry_.AddCommand(
      Command{ID_TRANSMISSION_VIEW}
          .set_execute_handler([this] {
            ::OpenView(main_window_,
                       MakeSingleWindowDefinition(&kTransmissionWindowInfo,
                                                  selection_->node()));
          })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   IsInstanceOf(selection_->node(), devices::id::DeviceType) &&
                   !IsInstanceOf(selection_->node(), devices::id::LinkType);
          }));

  command_registry_.AddCommand(
      Command{ID_COPY}
          .set_execute_handler([this] { CopyToClipboard(); })
          .set_enabled_handler([this] { return !selection_->empty(); })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   !selection_->empty();
          }));

  command_registry_.AddCommand(
      Command{ID_DELETE}
          .set_execute_handler([this] { DeleteSelection(); })
          .set_enabled_handler([this] { return !selection_->empty(); })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Configure) &&
                   !selection_->empty();
          }));

  command_registry_.AddCommand(
      Command{ID_WRITE}
          .set_execute_handler([this] {
            ExecuteWriteDialog(
                *dialog_service_,
                WriteContext{executor_, timed_data_service_,
                             selection_->node().node_id(), profile_, false});
          })
          .set_enabled_handler([this] {
            return !selection_->node()[data_items::id::DataItemType_Output]
                        .value()
                        .is_null();
          })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Control) &&
                   IsInstanceOf(selection_->node(),
                                data_items::id::DataItemType);
          }));

  command_registry_.AddCommand(
      Command{ID_WRITE_MANUAL}
          .set_execute_handler([this] {
            ExecuteWriteDialog(
                *dialog_service_,
                WriteContext{executor_, timed_data_service_,
                             selection_->node().node_id(), profile_, true});
          })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Control) &&
                   IsInstanceOf(selection_->node(),
                                data_items::id::DataItemType);
          }));

  command_registry_.AddCommand(
      Command{ID_UNLOCK_ITEM}
          .set_execute_handler([this] {
            auto node = selection_->node();
            task_manager_.PostTask(
                base::StringPrintf(u"Снятие блокировки с %ls",
                                   node.display_name().c_str()),
                [node] {
                  return node.scada_node().call(
                      data_items::id::DataItemType_Unlock);
                });
          })
          .set_enabled_handler([this] {
            return selection_->node()[data_items::id::DataItemType_Locked]
                .value()
                .get_or(false);
          })
          .set_available_handler([this] {
            return session_service_.HasPrivilege(scada::Privilege::Control) &&
                   IsInstanceOf(selection_->node(),
                                data_items::id::DataItemType);
          }));

  command_registry_.AddCommand(
      Command{ID_ACKNOWLEDGE_CURRENT}
          .set_execute_handler([this] {
            node_event_provider_.AcknowledgeItemEvents(
                selection_->node().node_id());
          })
          .set_enabled_handler(
              [this] { return selection_->timed_data().alerting(); })
          .set_available_handler(
              [this] { return selection_->timed_data().connected(); }));
}

void SelectionCommands::OpenWindow(const WindowInfo* window_info) {
  if (selection_ && !selection_->empty()) {
    // TODO: Capture |main_window_| by weak pointer.
    MakeWindowDefinition(window_info, selection_->node(), true)
        .then([weak_ptr = weak_ptr_factory_.GetWeakPtr()](
                  const WindowDefinition& window_definition) {
          if (auto ptr = weak_ptr.get())
            ptr->OpenWindow(window_definition);
        });
  }
}

void SelectionCommands::OpenWindow(const WindowDefinition& window_definition) {
  ::OpenView(main_window_, window_definition, true);
}

CommandHandler* SelectionCommands::GetCommandHandler(unsigned command_id) {
  if (!selection_ || !dialog_service_)
    return nullptr;

  return command_registry_.GetCommandHandler(command_id);
}

void SelectionCommands::CallMethod(
    const NodeRef& node,
    const scada::NodeId& method_id,
    const std::vector<scada::Variant>& arguments) {
  scada::BindStatusCallback(node.scada_node().call_packed(method_id, arguments),
                            [node, &local_events = local_events_,
                             &profile = profile_](const scada::Status& status) {
                              auto title = ToString16(node.display_name());
                              ReportRequestResult(title, status, local_events,
                                                  profile);
                            });
}

#if !defined(UI_WT)
void SelectionCommands::OpenModusView(const NodeRef& node) {
  assert(main_window_);
  assert(dialog_service_);

  auto cached_items =
      file_cache_.GetList(ID_MODUS_VIEW).GetFilesContainingItem(node.node_id());

  if (cached_items.empty()) {
    auto msg = base::StringPrintf(u"Схема для объекта \"%ls\" не найдена.",
                                  ToString16(node.display_name()).c_str());
    dialog_service_->RunMessageBox(msg, u"Схема", MessageBoxMode::Info);
    return;
  }

  // TODO: Let user select scheme from list.
  const FileCache::DisplayItem& cached_item = cached_items.front();
  const std::filesystem::path& path = cached_item.first;

  auto* view = main_window_manager_.FindOpenedViewByFilePath(path);
  if (view) {
    view->Activate();
  } else {
    WindowDefinition win(kModusWindowInfo);
    win.path = path;
    view = main_window_->OpenView(win, true);
  }

  if (view)
    view->SetSelection(node.node_id());
}
#endif

void SelectionCommands::SetContext(MainWindow* main_window,
                                   DialogService* dialog_service,
                                   Controller* controller,
                                   SelectionModel* selection) {
  main_window_ = main_window;
  dialog_service_ = dialog_service;
  controller_ = controller;
  selection_ = selection;
}

void SelectionCommands::DeleteSelection() {
  if (!session_service_.HasPrivilege(scada::Privilege::Configure))
    return;

  std::vector<NodeRef> nodes;

  if (selection_->multiple()) {
    auto node_ids = selection_->GetMultipleNodeIds();
    nodes.reserve(node_ids.size());
    std::transform(
        node_ids.begin(), node_ids.end(), std::back_inserter(nodes),
        [&node_service = node_service_](const scada::NodeId& node_id) {
          return node_service.GetNode(node_id);
        });
  } else if (auto node = selection_->node()) {
    nodes.emplace_back(std::move(node));
  }

  if (nodes.empty())
    return;

  auto message =
      nodes.size() == 1
          ? base::StringPrintf(u"Вы действительно хотите удалить %ls?",
                               nodes.front().display_name().c_str())
          : base::StringPrintf(u"Вы действительно хотите удалить %Iu объектов?",
                               nodes.size());

  dialog_service_
      ->RunMessageBox(message, u"Удаление", MessageBoxMode::QuestionYesNo)
      .then(BindPromiseExecutor(
          executor_, [&task_manager = task_manager_,
                      nodes = std::move(nodes)](MessageBoxResult result) {
            if (result != MessageBoxResult::Yes)
              return;
            for (auto& node : nodes)
              DeleteTreeRecordsRecursive(task_manager, node);
          }));
}

void SelectionCommands::CopyToClipboard() {
  std::vector<NodeRef> nodes;

  if (selection_->multiple()) {
    for (const auto& node_id : selection_->GetMultipleNodeIds()) {
      const auto& node = node_service_.GetNode(node_id);
      nodes.emplace_back(node);
      GetNodesRecursive(node, nodes);
    }

  } else if (const auto& node = selection_->node()) {
    nodes.emplace_back(node);
  }

  if (!nodes.empty())
    CopyNodesToClipboard(nodes);
}

promise<WindowDefinition> SelectionCommands::GetOpenWindowDefinition(
    const WindowInfo* window_info) const {
  if (auto open_context = controller_->GetOpenContext();
      open_context.has_value()) {
    return MakeWindowDefinition(window_info, open_context.value());
  }

  if (selection_->multiple()) {
    auto title = selection_->GetTitle();
    auto node_ids = selection_->GetMultipleNodeIds();
    return make_resolved_promise(MakeWindowDefinition(
        window_info, {node_ids.begin(), node_ids.end()}, title));
  }

  const auto& node_id = selection_->node().node_id();
  if (node_id.is_null() && selection_->timed_data().connected()) {
    const std::string& formula = selection_->timed_data().formula();
    return make_resolved_promise(MakeWindowDefinition(window_info, formula));
  }

  return MakeWindowDefinition(window_info, selection_->node(), true);
}

void SelectionCommands::DumpDebugInfo() {
  std::vector<std::string> debug_info;

  if (auto node = selection_->node())
    debug_info.push_back(GetNodeDebugInfo(node));

  debug_info.push_back(selection_->timed_data().DumpDebugInfo());

  auto debug_text = base::JoinString(debug_info, "\n\n");

  Clipboard clipboard;
  if (!clipboard.SetText(debug_text))
    LOG(WARNING) << "Can't set clipboard data";
  if (!clipboard.SetText(base::UTF8ToWide(debug_text)))
    LOG(WARNING) << "Can't set clipboard data";

  dialog_service_->RunMessageBox(
      u"Отладочная информация скопирована в буфер обмена.", {},
      MessageBoxMode::Info);
}
