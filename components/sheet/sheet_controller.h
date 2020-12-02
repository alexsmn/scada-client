#pragma once

#include "command_handler.h"
#include "contents_model.h"
#include "controller.h"
#include "controller_context.h"
#include "controls/color.h"
#include "selection_model.h"

#if defined(UI_VIEWS)
#include "ui/views/context_menu_controller.h"
#include "ui/views/controls/grid/grid_controller.h"
#include "ui/views/controls/textfield/textfield_controller.h"
#include "ui/views/drop_controller.h"
#endif

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

class SheetController : protected ControllerContext,
                        public Controller,
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
  explicit SheetController(const ControllerContext& context);

  // Controller
  virtual bool CanClose() const override;
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual bool IsCommandChecked(unsigned command_id) const override;
  virtual void ExecuteCommand(unsigned command) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
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

  void ChooseSelectionColor();
  void SetSelectionColor(aui::Color color);

  NodeIdSet GetSelectedNodeIdList();

  void OnFormulaEdited();
  void OnSelectionChanged();

#if defined(UI_VIEWS)
  // GridController
  virtual void OnGridGetAutocompleteList(
      views::GridView& sender,
      const std::wstring& text,
      int& start,
      std::vector<std::wstring>& list) override;
  virtual bool OnKeyPressed(views::GridView& sender,
                            ui::KeyboardCode key_code) override;
  virtual bool OnDoubleClick() override;

  // views::TextfieldController
  virtual void ContentsChanged(views::Textfield* sender,
                               const std::wstring& new_contents) override;
#endif

  SelectionModel selection_{{timed_data_service_}};

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
