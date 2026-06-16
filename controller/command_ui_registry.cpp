#include "controller/command_ui_registry.h"

#include <algorithm>

CommandDescriptor& UiCommandRegistry::RegisterCommand(
    CommandDescriptor descriptor,
    CommandPlacements placements) {
  descriptor.show_in_toolbar = placements.toolbar;
  descriptor.show_in_context_menu = placements.context_menu;

  auto& registered = command_manager_.RegisterCommand(std::move(descriptor));
  registered.show_in_toolbar = placements.toolbar;
  registered.show_in_context_menu = placements.context_menu;

  for (auto& contribution : placements.main_menu) {
    contribution.command_id = registered.command_id;
    AddMenuItem(std::move(contribution));
  }

  return registered;
}

void UiCommandRegistry::RegisterHandler(unsigned command_id,
                                        CommandContextId context_id,
                                        CommandHandler& handler) {
  command_manager_.RegisterHandler(command_id, context_id, handler);
}

void UiCommandRegistry::AddAction(Action action) {
  RegisterCommand(ToCommandDescriptor(action));
  action_manager_.AddAction(std::move(action));
}

void UiCommandRegistry::AddMenuItem(MenuContribution contribution) {
  menu_contributions_.emplace_back(std::move(contribution));
}

std::vector<MenuContribution> UiCommandRegistry::GetMenuContributions(
    MainMenuId menu_id) const {
  std::vector<MenuContribution> result;
  for (const auto& contribution : menu_contributions_) {
    if (contribution.menu_id == menu_id) {
      result.emplace_back(contribution);
    }
  }

  std::ranges::stable_sort(result, {}, &MenuContribution::order);
  return result;
}
