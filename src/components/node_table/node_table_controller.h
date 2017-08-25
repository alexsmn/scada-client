#pragma once

#include "base/win/dragdrop.h"
#include "client/controller.h"
#include "common/node_ref.h"

#if defined(UI_QT)
#elif defined(UI_VIEWS)
#include "ui/views/controls/grid/grid_controller.h"
#include "ui/views/controls/grid/grid_view.h"
#endif

#if defined(UI_QT)
class QTableView;
#elif defined(UI_VIEWS)
#endif

class NodeTableModel;

class NodeTableController : public Controller,
#if defined(UI_VIEWS)
                            private views::GridController,
#endif
                            public DataObject {
 public:
  NodeTableController(const ControllerContext& context, const scada::NodeId& parent_id);
  virtual ~NodeTableController();

  // Controller events
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual bool IsCommandEnabled(unsigned command_id) const override;
  virtual void ExecuteCommand(unsigned command_id) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual NodeRef GetRootNode() const override;

 protected:
#if defined(UI_VIEWS)
  // GridController overrides
  virtual bool CanEditCell(views::GridView& sender, int row, int column) override;
  virtual void OnGridSelectionChanged(views::GridView& sender) override;
  virtual bool OnGridEditCellText(views::GridView& sender, int row, int column,
                                  const base::string16& text) override;
  virtual views::ComboTextfield* OnGridCreateEditor(views::GridView& sender,
                                                    int row, int column) override;
  virtual void ShowContextMenu(gfx::Point point) override;
  virtual bool OnKeyPressed(views::GridView& sender, ui::KeyboardCode key_code) override;
#endif

 private:
  void DeleteSelection();
  void CopyToClipboard();
  void PasteFromClipboard();

  std::unique_ptr<NodeTableModel> model_;

#if defined(UI_QT)
  std::unique_ptr<QTableView> table_;

#elif defined(UI_VIEWS)
  std::unique_ptr<views::GridView> table_;
#endif
};
