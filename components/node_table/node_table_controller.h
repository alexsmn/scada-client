#pragma once

#include "base/memory/weak_ptr.h"
#include "base/win/dragdrop.h"
#include "command_registry.h"
#include "controller.h"
#include "controller_context.h"
#include "selection_model.h"

#if defined(UI_VIEWS)
#include "ui/views/controls/grid/grid_controller.h"
#endif

namespace aui {
class Grid;
}

class NodeTableModel;

class NodeTableController : protected ControllerContext,
                            public Controller,
#if defined(UI_VIEWS)
                            private views::GridController,
#endif
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

 protected:
#if defined(UI_VIEWS)
  // GridController overrides
  virtual bool OnKeyPressed(views::GridView& sender,
                            ui::KeyboardCode key_code) override;
  virtual void ShowHeaderContextMenu(gfx::Point point) override;
  virtual void OnGridColumnClicked(views::GridView& sender, int index) override;
#endif

 private:
  void SetSorting(const scada::NodeId& property_id);

  SelectionModel selection_{{timed_data_service_}};

  const std::shared_ptr<NodeTableModel> model_;
  aui::Grid* grid_ = nullptr;

  CommandRegistry command_registry_;

  base::WeakPtrFactory<NodeTableController> weak_ptr_factory_{this};
};
