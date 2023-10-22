#pragma once

#include "base/memory/weak_ptr.h"
#include "base/win/dragdrop.h"
#include "controller/command_registry.h"
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
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual NodeRef GetRootNode() const override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
  virtual bool IsWorking() const override;

 private:
  void SetSorting(const scada::NodeId& property_id);

  SelectionModel selection_{{timed_data_service_}};

  const std::shared_ptr<NodeTableModel> model_;
  aui::Grid* grid_ = nullptr;

  CommandRegistry command_registry_;

  base::WeakPtrFactory<NodeTableController> weak_ptr_factory_{this};
};
