#pragma once

#include "command_registry.h"
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
#elif defined(UI_WT)
class WContainerWidget;
class WLineEdit;
#elif defined(UI_VIEWS)
namespace views {
class Textfield;
}
#endif

class SheetModel;
class Grid;

class SheetController : protected ControllerContext,
                        public Controller,
                        public ContentsModel
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

  NodeIdSet GetSelectedNodeIdList();

  void OnFormulaEdited();
  void OnSelectionChanged();

#if defined(UI_VIEWS)
  // GridController
  virtual void OnGridGetAutocompleteList(
      views::GridView& sender,
      const std::u16string& text,
      int& start,
      std::vector<std::u16string>& list) override;
  virtual bool OnKeyPressed(views::GridView& sender,
                            ui::KeyboardCode key_code) override;
  virtual bool OnDoubleClick() override;

  // views::TextfieldController
  virtual void ContentsChanged(views::Textfield* sender,
                               const std::u16string& new_contents) override;
#endif

  std::shared_ptr<SheetModel> model_;

  SelectionModel selection_{{timed_data_service_}};

#if defined(UI_QT)
  QWidget* contents_view_ = nullptr;
  QLineEdit* formula_row_ = nullptr;
#elif defined(UI_WT)
  Wt::WContainerWidget* contents_view_ = nullptr;
  Wt::WLineEdit* formula_row_ = nullptr;
#elif defined(UI_VIEWS)
  ContentsView* contents_view_ = nullptr;
  views::Textfield* formula_row_ = nullptr;
#endif

  Grid* grid_ = nullptr;

  CommandRegistry command_registry_;
};
