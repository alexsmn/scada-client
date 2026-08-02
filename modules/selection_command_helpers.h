#pragma once

#include "base/any_executor.h"
#include "base/awaitable.h"
#include "base/u16format.h"
#include "common/formula_util.h"
#include "controller/command_registry.h"
#include "controller/selection_model.h"
#include "controller/window_info.h"
#include "core/selection_command_context.h"
#include "main_window/main_window_interface.h"
#include "main_window/opened_view/opened_view_interface.h"
#include "node_service/node_ref.h"
#include "profile/window_definition.h"
#include "ui/common/client_utils.h"

#include <functional>

inline WindowDefinition MakeSelectionSingleWindowDefinition(
    const WindowInfo& window_info,
    const NodeRef& node) {
  WindowDefinition window_def{window_info};
  window_def.title =
      u16format(L"{}: {}", window_info.title, ToString16(node.display_name()));
  window_def.AddItem("Item").SetString("path",
                                       MakeNodeIdFormula(node.node_id()));
  return window_def;
}

inline BasicCommand<SelectionCommandContext> MakeOpenViewSelectionCommand(
    unsigned command_id,
    const WindowInfo& window_info,
    AnyExecutor executor) {
  return BasicCommand<SelectionCommandContext>{
      .command_id = command_id,
      .execute_handler =
          [&window_info, executor = std::move(executor)](
              const SelectionCommandContext& context) {
            auto window_def =
                context.opened_view.GetOpenWindowDefinition(&window_info);
            CoSpawn(
                executor,
                [&main_window = context.main_window,
                 window_def =
                     std::move(window_def)]() mutable -> Awaitable<void> {
                  co_await main_window.OpenView(co_await std::move(window_def));
                  co_return;
                });
          },
      .available_handler =
          [](const SelectionCommandContext& context) {
            return !context.selection.empty();
          }};
}

inline BasicCommand<SelectionCommandContext> MakeOpenSingleSelectionCommand(
    unsigned command_id,
    const WindowInfo& window_info,
    AnyExecutor executor,
    std::function<bool(const SelectionCommandContext&)> available_handler) {
  return BasicCommand<SelectionCommandContext>{
      .command_id = command_id,
      .execute_handler =
          [&window_info, executor = std::move(executor)](
              const SelectionCommandContext& context) {
            CoSpawn(executor,
                    [&main_window = context.main_window,
                     window_definition = MakeSelectionSingleWindowDefinition(
                         window_info,
                         context.selection.node())]() -> Awaitable<void> {
                      co_await main_window.OpenView(window_definition);
                      co_return;
                    });
          },
      .available_handler = std::move(available_handler)};
}
