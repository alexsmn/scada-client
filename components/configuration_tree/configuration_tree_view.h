#pragma once

#include <memory>

#include "controller.h"

#if defined(UI_VIEWS)
#include "ui/views/drop_controller.h"
#include "ui/views/controls/tree/tree_controller.h"
#endif

namespace ui {
class SortedTreeModel;
}

class ConfigurationTreeModel;
class Tree;

using DropAction = std::function<int()>;

class ConfigurationTreeView : public Controller
#if defined(UI_VIEWS)
    ,
                              protected views::DropController
#endif
{
 public:
  ConfigurationTreeView(const ControllerContext& context,
                        ConfigurationTreeModel& model);
  virtual ~ConfigurationTreeView();

  // View
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command) override;
  virtual void OnViewNodeCreated(const NodeRef& node) override;
#if defined(UI_VIEWS)
  virtual views::DropController* GetDropController() override { return this; }
#endif

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

  std::unique_ptr<ConfigurationTreeModel> model_;

  scada::NodeId dragging_item_id_;
  DropAction drop_action_;

  std::unique_ptr<Tree> tree_view_;
};
