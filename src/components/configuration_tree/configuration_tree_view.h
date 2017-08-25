#pragma once

#include <memory>

#include "client/controller.h"
#include "ui/views/controls/tree/tree_controller.h"
#include "ui/views/drop_controller.h"

namespace ui {
class SortedTreeModel;
}

class ConfigurationTreeModel;
class Tree;

class ConfigurationTreeView : public Controller,
                              protected views::DropController {
 public:
  ConfigurationTreeView(const ControllerContext& context, std::unique_ptr<ConfigurationTreeModel> model);
  virtual ~ConfigurationTreeView();

  // View
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command) override;
  virtual void OnViewNodeCreated(const NodeRef& node) override;

 protected:
  ConfigurationTreeModel& model() { return *model_; }

  Tree& tree_view() { return *tree_view_; }

#if defined(UI_QT)
#elif defined(UI_VIEWS)
  void StartDrag(void* node);

  // DragTarget events
  // TODO: Repair.
  virtual bool CanDrop(const ui::OSExchangeData& data) override;
  virtual void OnDragEntered(const ui::DropTargetEvent& event) override;
  virtual int OnDragUpdated(const ui::DropTargetEvent& event) override;
  virtual void OnDragDone() override;
  virtual int OnPerformDrop(const ui::DropTargetEvent& event) override;
#endif

 private:
  void DeleteSelection();

#if defined(UI_VIEWS)
  void* TestDrop(const ui::DropTargetEvent& event) const;
#endif

  std::unique_ptr<ConfigurationTreeModel> model_;
  std::unique_ptr<ui::SortedTreeModel> sorted_model_;

  scada::NodeId dragging_item_id_;

  std::unique_ptr<Tree> tree_view_;
};
