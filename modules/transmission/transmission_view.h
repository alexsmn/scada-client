#pragma once

#include "controller/command_registry.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/selection_model.h"

#include <memory>

namespace scada::aui {
class ColumnHeaderModel;
class Grid;
}  // namespace scada::aui

class TransmissionModel;

class TransmissionView : protected ControllerContext, public Controller {
 public:
  explicit TransmissionView(const ControllerContext& context);
  virtual ~TransmissionView();

  // Controller events
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual ContentsModel* GetContentsModel() override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }

 private:
  void DeleteSelection();
  void OnSelectionChanged();

  const std::shared_ptr<TransmissionModel> model_;
  const std::shared_ptr<scada::aui::ColumnHeaderModel> column_model_;

  scada::aui::Grid* grid_ = nullptr;

  CommandRegistry command_registry_;

  SelectionModel selection_{{timed_data_service_}};
};
