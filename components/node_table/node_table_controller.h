#pragma once

#include "base/memory/weak_ptr.h"
#include "base/win/dragdrop.h"
#include "controller.h"

#if defined(UI_QT)
#elif defined(UI_VIEWS)
#include "ui/views/controls/grid/grid_controller.h"
#endif

class Grid;
class NodeTableModel;

class NodeTableController : public Controller,
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
  virtual bool IsCommandEnabled(unsigned command_id) const override;
  virtual bool IsCommandChecked(unsigned command_id) const override;
  virtual void ExecuteCommand(unsigned command_id) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual NodeRef GetRootNode() const override;

 protected:
#if defined(UI_VIEWS)
  // GridController overrides
  virtual void OnGridSelectionChanged(views::GridView& sender) override;
  virtual bool OnKeyPressed(views::GridView& sender,
                            ui::KeyboardCode key_code) override;
  virtual void ShowHeaderContextMenu(gfx::Point point) override;
  virtual void OnGridColumnClicked(views::GridView& sender, int index) override;
#endif

 private:
  void SetSorting(const scada::NodeId& property_id);

  std::unique_ptr<NodeTableModel> model_;
  std::unique_ptr<Grid> grid_;

  base::WeakPtrFactory<NodeTableController> weak_ptr_factory_{this};
};
