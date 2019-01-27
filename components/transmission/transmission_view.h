#pragma once

#include "command_handler.h"
#include "controller.h"
#include "ui/base/models/header_model.h"

#include <memory>

class Grid;
class TransmissionModel;

class TransmissionView : public Controller, public CommandHandler {
 public:
  explicit TransmissionView(const ControllerContext& context);
  virtual ~TransmissionView();

  // Controller events
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command) override;
  virtual ContentsModel* GetContentsModel() override;

 private:
  void DeleteSelection();

  std::unique_ptr<TransmissionModel> model_;
  ui::ColumnHeaderModel column_model_;

  std::unique_ptr<Grid> grid_;
};
