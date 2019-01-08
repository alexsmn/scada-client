#pragma once

#include "controller.h"
#include "export_model.h"

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
  virtual bool IsCommandChecked(unsigned command_id) const override;
  virtual void ExecuteCommand(unsigned command) override;
  virtual ContentsModel* GetContentsModel() override;
  virtual TimeModel* GetTimeModel() override;
  virtual ExportModel* GetExportModel() override;

 private:
  void ExportToExcel();

  std::unique_ptr<SummaryModel> model_;
  std::unique_ptr<Grid> grid_;
};
