#include "debugger_module.h"

#include "aui/dialog_service.h"
#include "base/command_line.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/win/clipboard.h"
#include "common_resources.h"
#include "components/debugger/debug_switch.h"
#include "controller/command_registry.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "debugger.h"
#include "node_debug_info.h"

#include <memory>

DebuggerModule::DebuggerModule(DebuggerModuleContext&& context)
    : DebuggerModuleContext{std::move(context)} {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(kDebugSwitch)) {
    auto debugger = std::make_shared<Debugger>(
        DebuggerContext{.session_service_ = session_service_});

    main_commands_.AddCommand(
        {.title = u"Отладчик",
         .menu_group = MenuGroup::DEBUG,
         .execute_handler = [debugger](const MainCommandContext& context) {
           debugger->Open();
         }});

    selection_commands_.AddCommand(
        {.command_id = ID_DUMP_DEBUG_INFO,
         .execute_handler =
             [this](const SelectionCommandContext& context) {
               DumpDebugInfo(context);
             },
         .available_handler =
             [this](const SelectionCommandContext& context) {
               return context.selection.timed_data().connected();
             }});
  }
}

void DebuggerModule::DumpDebugInfo(const SelectionCommandContext& context) {
  std::vector<std::string> debug_info;

  if (auto node = context.selection.node()) {
    debug_info.push_back(GetNodeDebugInfo(node));
  }

  debug_info.push_back(context.selection.timed_data().DumpDebugInfo());

  auto debug_text = base::JoinString(debug_info, "\n\n");

  Clipboard clipboard;

  if (!clipboard.SetText(debug_text)) {
    LOG(WARNING) << "Can't set clipboard data";
  }

  if (!clipboard.SetText(base::UTF8ToWide(debug_text))) {
    LOG(WARNING) << "Can't set clipboard data";
  }

  context.dialog_service.RunMessageBox(
      u"Отладочная информация скопирована в буфер обмена.", {},
      MessageBoxMode::Info);
}
