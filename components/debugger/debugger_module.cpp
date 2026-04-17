#include "debugger_module.h"

#include "aui/dialog_service.h"
#include "aui/translation.h"
#include "base/boost_log.h"
#include "base/program_options.h"
#include <boost/algorithm/string/join.hpp>
#include "base/utf_convert.h"
#include "base/win/clipboard.h"
#include "resources/common_resources.h"
#include "components/debugger/debug_switch.h"
#include "controller/command_registry.h"
#include "controller/selection_model.h"
#include "core/selection_command_context.h"
#include "debugger.h"
#include "node_debug_info.h"

#include <memory>

DebuggerModule::DebuggerModule(DebuggerModuleContext&& context)
    : DebuggerModuleContext{std::move(context)} {
  if (client::HasOption(kDebugSwitch)) {
    auto debugger = std::make_shared<Debugger>(
        DebuggerContext{.session_service_ = session_service_});

    global_commands_.AddCommand(
        {.title = Translate("Debugger"),
         .menu_group = MenuGroup::DEBUG,
         .execute_handler = [debugger](const GlobalCommandContext& context) {
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

  auto debug_text = boost::algorithm::join(debug_info, "\n\n");

  Clipboard clipboard;

  if (!clipboard.SetText(debug_text)) {
    BOOST_LOG_TRIVIAL(warning) << "Can't set clipboard data";
  }

  if (!clipboard.SetText(UtfConvert<wchar_t>(debug_text))) {
    BOOST_LOG_TRIVIAL(warning) << "Can't set clipboard data";
  }

  context.dialog_service.RunMessageBox(
      Translate("Debug information copied to clipboard."), {},
      MessageBoxMode::Info);
}
