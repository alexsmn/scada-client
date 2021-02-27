#pragma once

#include "controller.h"
#include "controller_context.h"
#include "selection_model.h"

#if defined(UI_VIEWS)
#include "ui/views/controls/tree/tree_controller.h"
#endif

#include <memory>

namespace ui {
class SortedTreeModel;
}

class ConfigurationTreeModel;
class ConfigurationTreeDropHandler;
class ConfigurationTreeDragDropControllerViews;
class Tree;

class ConfigurationTreeView : protected ControllerContext, public Controller {
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
  virtual views::DropController* GetDropController() override;
#endif

 protected:
  ConfigurationTreeModel& model() const { return *model_; }

  Tree& tree_view() const { return *tree_view_; }

  std::vector<scada::NodeId> GetVariableNodeIds(
      const std::vector<void*>& nodes) const;

 private:
  SelectionModel selection_{{timed_data_service_}};

  std::unique_ptr<ConfigurationTreeModel> model_;

  std::unique_ptr<ConfigurationTreeDropHandler> drop_handler_;

#if defined(UI_VIEWS)
  std::unique_ptr<ConfigurationTreeDragDropControllerViews> drag_drop_controller_;
#endif

  std::unique_ptr<Tree> tree_view_;
};
