#pragma once

#include "command_handler_impl.h"
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

  std::unique_ptr<TransmissionModel> model_;
  ui::ColumnHeaderModel column_model_;

  std::unique_ptr<Grid> grid_;

  CommandHandlerImpl command_handler_;
};
