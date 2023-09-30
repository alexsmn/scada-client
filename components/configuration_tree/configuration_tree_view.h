#pragma once

#include "aui/handlers.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "selection_model.h"

#include <memory>

namespace aui {
class Tree;
}

class ConfigurationTreeModel;
class ConfigurationTreeDropHandler;

class ConfigurationTreeView : protected ControllerContext, public Controller {
 public:
  ConfigurationTreeView(
      const ControllerContext& context,
      std::shared_ptr<ConfigurationTreeModel> model,
      std::unique_ptr<ConfigurationTreeDropHandler> drop_handler);
  virtual ~ConfigurationTreeView();

  // View
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual void OnViewNodeCreated(const NodeRef& node) override;
  virtual std::optional<OpenContext> GetOpenContext() const override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }

 protected:
  ConfigurationTreeModel& model() const { return *model_; }

  aui::Tree& tree_view() const { return *tree_view_; }

  std::vector<scada::NodeId> GetVariableNodeIds(
      const std::vector<void*>& nodes) const;

 private:
  void UpdateSelection();

  DragData GetDragData(const std::vector<void*>& nodes) const;

  const std::shared_ptr<ConfigurationTreeModel> model_;

  SelectionModel selection_{{timed_data_service_}};

  std::unique_ptr<ConfigurationTreeDropHandler> drop_handler_;

  aui::Tree* tree_view_ = nullptr;
};
