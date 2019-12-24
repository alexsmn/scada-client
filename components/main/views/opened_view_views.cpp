#include "components/main/opened_view.h"

#include "model/scada_node_ids.h"
#include "common_resources.h"
#include "components/main/views/main_window_views.h"
#include "contents_model.h"
#include "controller.h"
#include "views/client_utils_views.h"
#include "views/item_drag_data.h"
#include "window_info.h"

void OpenedView::ShowPopupMenu(unsigned resource_id,
                               const gfx::Point& point,
                               bool right_click) {
  assert(main_window_);

  if (resource_id == 0)
    resource_id = window_info().menu;
  if (resource_id == 0)
    resource_id = IDR_ITEM_POPUP;

  // TODO: Avoid the cast.
  HMENU menu = CreatePopupMenu(resource_id, context_menu_model_);
  if (!menu)
    return;

  HMENU popup_menu = GetSubMenu(menu, 0);
  assert(popup_menu);
  ::ShowPopupMenu(dialog_service_.GetDialogOwningWindow(), popup_menu, point,
                  right_click);

  DestroyMenu(menu);
}

bool OpenedView::CanDrop(const ui::OSExchangeData& data) {
  if (auto* drop_controller = controller_->GetDropController()) {
    if (drop_controller->CanDrop(data))
      return true;
  }

  ItemDragData item_data;
  if (item_data.Load(data))
    return window_info().can_insert_item();

  return false;
}

void OpenedView::OnDragEntered(const ui::DropTargetEvent& event) {
  if (auto* drop_controller = controller_->GetDropController())
    drop_controller->OnDragEntered(event);
}

int OpenedView::OnDragUpdated(const ui::DropTargetEvent& event) {
  if (auto* drop_controller = controller_->GetDropController()) {
    auto controller_result = drop_controller->OnDragUpdated(event);
    if (controller_result != ui::DragDropTypes::DRAG_NONE)
      return controller_result;
  }

  ItemDragData item_data;
  if (item_data.Load(event.data())) {
    if (window_info().can_insert_item())
      return ui::DragDropTypes::DRAG_COPY;
  }

  return ui::DragDropTypes::DRAG_NONE;
}

void OpenedView::OnDragDone() {
  if (auto* drop_controller = controller_->GetDropController())
    drop_controller->OnDragDone();
}

int OpenedView::OnPerformDrop(const ui::DropTargetEvent& event) {
  if (auto* drop_controller = controller_->GetDropController()) {
    auto controller_result = drop_controller->OnPerformDrop(event);
    if (controller_result != ui::DragDropTypes::DRAG_NONE)
      return controller_result;
  }

  ItemDragData item_data;
  if (item_data.Load(event.data())) {
    if (window_info().can_insert_item()) {
      auto* contents_model = controller_->GetContentsModel();
      if (contents_model) {
        contents_model->AddContainedItem(item_data.item_id(),
                                         ContentsModel::APPEND);
        return ui::DragDropTypes::DRAG_COPY;
      }
    }
  }

  return ui::DragDropTypes::DRAG_NONE;
}
