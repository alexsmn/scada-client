#pragma once

#include "controls/handlers.h"
#include "core/node_id.h"
#include "ui/views/drop_controller.h"

namespace scada {
class SessionService;
}

namespace views {
class TreeView;
}

class ConfigurationTreeView;
class ConfigurationTreeDropHandler;

class ConfigurationTreeDragDropControllerViews : public views::DropController {
 public:
  ConfigurationTreeDragDropControllerViews(
      views::TreeView& tree_view,
      ConfigurationTreeDropHandler& drop_handler,
      scada::SessionService& session_service);

  void StartDrag(void* node);

  // views::DropController
  virtual bool CanDrop(const ui::OSExchangeData& data) override;
  virtual void OnDragEntered(const ui::DropTargetEvent& event) override;
  virtual int OnDragUpdated(const ui::DropTargetEvent& event) override;
  virtual void OnDragDone() override;
  virtual int OnPerformDrop(const ui::DropTargetEvent& event) override;

 private:
  views::TreeView& tree_view_;
  ConfigurationTreeDropHandler& drop_handler_;
  scada::SessionService& session_service_;

  scada::NodeId dragging_item_id_;
  DropAction drop_action_;
};
