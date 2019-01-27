#pragma once

#include "command_handler.h"
#include "contents_model.h"
#include "controller.h"

#if defined(UI_VIEWS)
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/grid/grid_controller.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/drop_controller.h"
#endif

#include <SkColor.h>
#include <memory>

namespace ui {
class OSExchangeData;
}

#if defined(UI_QT)
class QLineEdit;
class QWidget;
#elif defined(UI_VIEWS)
namespace views {
class Textfield;
}
#endif

class SheetModel;
class Grid;

class SheetView : public Controller,
                  public ContentsModel,
                  public CommandHandler
#if defined(UI_VIEWS)
    ,
                  private views::GridController,
                  private views::TextfieldController,
                  private views::DropController
#endif
{
 public:
  explicit SheetView(const ControllerContext& context);

  // Controller
  virtual bool CanClose() const override;
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual bool IsCommandChecked(unsigned command_id) const override;
  virtual void ExecuteCommand(unsigned command) override;
  virtual ContentsModel* GetContentsModel() override { return this; }
#if defined(UI_VIEWS)
  virtual views::DropController* GetDropController() override { return this; }
#endif

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;

 protected:
#if defined(UI_VIEWS)
  // View
  virtual bool CanDrop(const ui::OSExchangeData& data) override;
  virtual int OnPerformDrop(const ui::DropTargetEvent& event) override;
#endif

 private:
  friend class SheetCell;

  class ContentsView;

  void UpdateEditing();
  void UpdateFormulaRow();
  void ClearSelection();

  void SetSelectionColor(SkColor color);

  NodeIdSet GetSelectedNodeIdList();

#if defined(UI_VIEWS)
  // GridController
  virtual void OnGridGetAutocompleteList(
      views::GridView& sender,
      const base::string16& text,
      int& start,
      std::vector<base::string16>& list) override;
  virtual void OnGridSelectionChanged(views::GridView& sender) override;
  virtual bool OnKeyPressed(views::GridView& sender,
                            ui::KeyboardCode key_code) override;
  virtual bool OnDoubleClick() override;

  // views::TextfieldController
  virtual void ContentsChanged(views::Textfield* sender,
                               const base::string16& new_contents) override;
#endif

  std::unique_ptr<SheetModel> model_;

#if defined(UI_QT)
  std::unique_ptr<QWidget> contents_view_;
  std::unique_ptr<QLineEdit> formula_row_;
#elif defined(UI_VIEWS)
  std::unique_ptr<ContentsView> contents_view_;
  std::unique_ptr<views::Textfield> formula_row_;
#endif

  std::unique_ptr<Grid> grid_;
};
