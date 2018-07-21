#pragma once

#include "controller.h"

#include <memory>

class Grid;
class SummaryModel;

class SummaryView : public Controller {
 public:
  explicit SummaryView(const ControllerContext& context);

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command) override;
  virtual TimeModel* GetTimeModel() override;

 private:
  void ExportToExcel();

  std::unique_ptr<SummaryModel> model_;
  std::unique_ptr<Grid> grid_;
};
