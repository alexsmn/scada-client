#pragma once

#include "command_registry.h"
#include "contents_model.h"
#include "controller.h"
#include "controller_context.h"
#include "controls/color.h"
#include "selection_model.h"

#include <memory>

namespace aui {
class Grid;
}

namespace ui {
class OSExchangeData;
}

#if defined(UI_QT)
class QLineEdit;
class QWidget;
#elif defined(UI_WT)
class WContainerWidget;
class WLineEdit;
#endif

class SheetModel;

class SheetController : protected ControllerContext,
                        public Controller,
                        public ContentsModel {
 public:
  explicit SheetController(const ControllerContext& context);

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
  virtual ContentsModel* GetContentsModel() override { return this; }

  // ContentsModel
  virtual void AddContainedItem(const scada::NodeId& node_id,
                                unsigned flags) override;

 private:
  class ContentsView;

  void UpdateEditing();
  void UpdateFormulaRow();
  void ClearSelection();

  void ChooseSelectionColor();

  NodeIdSet GetSelectedNodeIdList();

  void OnFormulaEdited();
  void OnSelectionChanged();

  std::shared_ptr<SheetModel> model_;

  SelectionModel selection_{{timed_data_service_}};

#if defined(UI_QT)
  QWidget* contents_view_ = nullptr;
  QLineEdit* formula_row_ = nullptr;
#elif defined(UI_WT)
  Wt::WContainerWidget* contents_view_ = nullptr;
  Wt::WLineEdit* formula_row_ = nullptr;
#endif

  aui::Grid* grid_ = nullptr;

  CommandRegistry command_registry_;

  friend class SheetCell;
};
