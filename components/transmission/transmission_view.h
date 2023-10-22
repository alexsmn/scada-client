#pragma once

#include "controller/command_registry.h"
#include "controller/controller.h"
#include "controller/controller_context.h"

#include <memory>

namespace aui {
class ColumnHeaderModel;
class Grid;
}  // namespace aui

class TransmissionModel;

class TransmissionView : protected ControllerContext, public Controller {
 public:
  explicit TransmissionView(const ControllerContext& context);
  virtual ~TransmissionView();

  // Controller events
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual ContentsModel* GetContentsModel() override;

 private:
  void DeleteSelection();

  const std::shared_ptr<TransmissionModel> model_;
  const std::shared_ptr<aui::ColumnHeaderModel> column_model_;

  aui::Grid* grid_ = nullptr;

  CommandRegistry command_registry_;
};
