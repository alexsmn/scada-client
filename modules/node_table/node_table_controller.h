#pragma once

#include "base/win/dragdrop.h"
#include "controller/action_manager.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/selection_model.h"

namespace aui {
class Grid;
}

class NodeTableModel;

class NodeTableController : protected ControllerContext,
                            public Controller,
                            public DataObject {
 public:
  NodeTableController(const ControllerContext& context,
                      const NodeRef& parent_node);
  virtual ~NodeTableController();

  // Controller events
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual ActionManager* GetActionManager() override;
  virtual void Save(WindowDefinition& definition) override;
  virtual NodeRef GetRootNode() const override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
  virtual bool IsWorking() const override;

 private:
  void SetSorting(const scada::NodeId& property_id);

  SelectionModel selection_{{timed_data_service_}};

  const std::shared_ptr<NodeTableModel> model_;
  aui::Grid* grid_ = nullptr;

  ActionManager command_registry_;
};
