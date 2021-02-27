#pragma once

#include "controller.h"
#include "controller_context.h"
#include "selection_model.h"

#if defined(UI_VIEWS)
#include "ui/views/controls/tree/tree_controller.h"
#include "ui/views/drop_controller.h"
#endif

#include <memory>

namespace ui {
class SortedTreeModel;
}

class ConfigurationTreeModel;
class ConfigurationTreeDropHandler;
class Tree;

class ConfigurationTreeView : protected ControllerContext,
                              public Controller
#if defined(UI_VIEWS)
    ,
                              protected views::DropController
#endif
{
 public:
  ConfigurationTreeView(const ControllerContext& context,
                        ConfigurationTreeModel& model,
                        ConfigurationTreeDropHandler& drop_handler);
  virtual ~ConfigurationTreeView();

  // View
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual void OnViewNodeCreated(const NodeRef& node) override;
  virtual std::optional<OpenContext> GetOpenContext() const override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
#if defined(UI_VIEWS)
  virtual views::DropController* GetDropController() override { return this; }
#endif

 protected:
  ConfigurationTreeModel& model() const { return *model_; }

  Tree& tree_view() const { return *tree_view_; }

  std::vector<scada::NodeId> GetVariableNodeIds(
      const std::vector<void*>& nodes) const;

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
  SelectionModel selection_{{timed_data_service_}};

  std::unique_ptr<ConfigurationTreeModel> model_;
  std::unique_ptr<ConfigurationTreeDropHandler> drop_handler_;

  scada::NodeId dragging_item_id_;
  DropAction drop_action_;

  std::unique_ptr<Tree> tree_view_;
};
