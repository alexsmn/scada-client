#include "components/main/opened_view.h"

#include "views/client_utils_views.h"
#include "common_resources.h"
#include "views/item_drag_data.h"
#include "controller.h"
#include "contents_model.h"
#include "window_info.h"
#include "common/scada_node_ids.h"

void OpenedView::ShowPopupMenu(unsigned resource_id, const gfx::Point& point, bool right_click) {
  HMENU menu = CreatePopupMenu(resource_id, *this, action_manager_);
  if (!menu)
    return;

  HMENU popup_menu = GetSubMenu(menu, 0);
  assert(popup_menu);
  ::ShowPopupMenu(main_window_, popup_menu, point, right_click);

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
    auto result = drop_controller->OnDragUpdated(event);
    if (result != ui::DragDropTypes::DRAG_NONE)
      return result;
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
    auto result = drop_controller->OnPerformDrop(event);
    if (result != ui::DragDropTypes::DRAG_NONE)
      return result;
  }

  ItemDragData item_data;
  if (item_data.Load(event.data())) {
    if (window_info().can_insert_item()) {
      auto* contents_model = controller_->GetContentsModel();
      if (contents_model) {
        contents_model->AddContainedItem(item_data.item_id(), ContentsModel::APPEND);
        return ui::DragDropTypes::DRAG_COPY;
      }
    }
  }

  return ui::DragDropTypes::DRAG_NONE;
}

void OpenedView::Print() {
  /*assert(active_view());

  CPrinter printer;
  printer.OpenDefaultPrinter();
  CDC printer_dc = printer.CreatePrinterDC();
  if (!printer_dc)
    return;	// TODO: message

  int iWidth = printer_dc.GetDeviceCaps(PHYSICALWIDTH); 
  int iHeight = printer_dc.GetDeviceCaps(PHYSICALHEIGHT); 
  int nLogx = printer_dc.GetDeviceCaps(LOGPIXELSX);
  int nLogy = printer_dc.GetDeviceCaps(LOGPIXELSY);

  RECT rcMM = { 0, 0, ::MulDiv(iWidth, 2540, nLogx), ::MulDiv(iHeight, 2540, nLogy) };

  CEnhMetaFileDC meta_dc(printer_dc, &rcMM);
  if (!active_view()->Print((HDC)meta_dc))
    return;

  WindowDefinition def(GetWindowInfo(ID_PRINT_PREVIEW));
  PrintPreviewView& view = static_cast<PrintPreviewView&>(OpenView(def));
  view.m_sizeCurPhysOffset.cx = printer_dc.GetDeviceCaps(PHYSICALOFFSETX);
  view.m_sizeCurPhysOffset.cy = printer_dc.GetDeviceCaps(PHYSICALOFFSETY);
  view.SetEnhMetaFile(meta_dc.Close()); */
}

void OpenedView::ShowContextMenuForView(views::View* source, const gfx::Point& point) {
  auto menu_id = window_info().menu;
  if (menu_id == 0)
    menu_id = IDR_ITEM_POPUP;
  ShowPopupMenu(menu_id, point, true);
}
