#include "main_window/toolbar_controller_wt.h"

#include "base/any_executor_dispatch.h"
#include "controller/command_handler.h"

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <wt/WLayout.h>
#include <wt/WMenu.h>
#include <wt/WPushButton.h>
#pragma warning(pop)

ToolbarController::ToolbarController(ToolbarControllerContext&& context)
    : ToolbarControllerContext{std::move(context)} {}

std::unique_ptr<Wt::WToolBar> ToolbarController::CreateToolbar() {
  auto toolbar = std::make_unique<Wt::WToolBar>();

  for (auto* command_info : command_manager_.commands()) {
    // bool collapsible = !CanExpandCommandCategory(command_info->category);
    auto action = std::make_unique<Wt::WPushButton>();
    action->setText(command_info->GetShortTitle());
    action->hide();
    /*if (command_info->image_id != 0)
      action->setIcon(QIcon(LoadPixmap(command_info->image_id)));*/
    action->setCheckable(command_info->checkable());
    auto command_id = command_info->command_id;
    action->clicked().connect([this, command_id] {
      auto* handler = commands_.GetCommandHandler(command_id);
      if (handler && handler->IsCommandEnabled(command_id))
        handler->ExecuteCommand(command_id);
    });
    auto* action_ptr = action.get();
    toolbar->addButton(std::move(action));
    action_command_ids_.emplace(action_ptr, command_info->command_id);
    action_map_.emplace(command_info->command_id, action_ptr);
  }

  /*{
    // Action order is important.
    int last_category = -1;
    for (auto* action_info : action_manager_.actions()) {
      auto* action = FindAction(action_info->command_id());
      if (CanExpandCommandCategory(action_info->category_)) {
        toolbar_->addAction(action);
        if (last_category != -1 && last_category != action_info->category_) {
          toolbar_->addSeparator();
        }
      } else {
        auto& category_action = category_actions_[action_info->category_];
        if (!category_action.menu) {
          auto* button = new QToolButton(toolbar_.get());
          auto menu = std::make_unique<QMenu>();
          auto text = QString::fromWCharArray(
              GetCommandCategoryTitle(action_info->category_));
          button->setMenu(menu.get());
          button->setPopupMode(QToolButton::InstantPopup);
          button->setText(text);
          QObject::connect(
              menu.get(), &QMenu::aboutToShow,
              [this, &menu = *menu.get()] { UpdateMenuActions(menu); });
          category_action.menu = std::move(menu);
          category_action.toolbar_action = toolbar_->addWidget(button);
        }
        category_action.menu->addAction(action);
      }
      last_category = action_info->category_;
    }
  }*/

  return toolbar;
}

ToolbarController::~ToolbarController() {}

void ToolbarController::OnActionChanged(Action& action,
                                        ActionChangeMask change_mask) {}

void ToolbarController::OnSelectionChanged() {
  for (auto& p : action_map_)
    UpdateAction(*p.second, p.first, ActionChangeMask::AllButTitle);
}

void ToolbarController::UpdateAction(Wt::WPushButton& action,
                                     unsigned command_id,
                                     ActionChangeMask change_mask) {
  if (static_cast<unsigned>(change_mask) &
      static_cast<unsigned>(ActionChangeMask::Title)) {
    if (auto* a = action_manager_.FindAction(command_id))
      action.setText(a->GetTitle());
  }

  auto* handler = commands_.GetCommandHandler(command_id);
  if (handler)
    action.show();
  else
    action.hide();
  if (handler) {
    bool enabled = handler->IsCommandEnabled(command_id);
    action.setEnabled(enabled);
    if (enabled)
      action.setChecked(handler->IsCommandChecked(command_id));
  }
}
