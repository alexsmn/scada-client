#pragma once

#include "aui/color.h"
#include "controller/command_registry.h"
#include "controller/contents_model.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/selection_model.h"
#include "modules/sheet/sheet_menu_model.h"

#include <memory>

namespace scada::aui {
class Grid;
}

namespace scada::ui {
class OSExchangeData;
}

#if defined(UI_QT)
class QLineEdit;
class QWidget;
#endif

class SheetModel;

class SheetController : protected ControllerContext,
                        public Controller,
                        public ContentsModel {
 public:
  explicit SheetController(const ControllerContext& context);

  // Controller
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
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
#endif

  scada::aui::Grid* grid_ = nullptr;

  CommandRegistry command_registry_;

  // Cross-platform context menu, backed by `command_registry_`. Declared after
  // it so the registry outlives the menu's delegate.
  SheetMenuModel sheet_menu_model_{command_registry_};

  friend class SheetCell;
};
