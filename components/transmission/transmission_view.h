#pragma once

#include "command_registry.h"
#include "controller.h"
#include "controller_context.h"
#include "ui/base/models/header_model.h"

#include <memory>

class Grid;
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
  const std::shared_ptr<ui::ColumnHeaderModel> column_model_;

  Grid* grid_ = nullptr;

  CommandRegistry command_registry_;
};
