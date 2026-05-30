#include "controller/command_ui_registry.h"

#include <algorithm>

void UiCommandRegistry::AddAction(Action action) {
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
