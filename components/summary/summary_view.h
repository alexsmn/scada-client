#pragma once

#include "command_registry.h"
#include "controller.h"
#include "controller_context.h"
#include "export_model.h"
#include "selection_model.h"

#include <memory>

class Grid;
class SummaryModel;

class SummaryView : protected ControllerContext, public Controller {
 public:
  explicit SummaryView(const ControllerContext& context);

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
  virtual ContentsModel* GetContentsModel() override;
  virtual TimeModel* GetTimeModel() override;
  virtual ExportModel* GetExportModel() override;
  virtual std::optional<OpenContext> GetOpenContext() const override;

 private:
  SelectionModel selection_{{timed_data_service_}};

  const std::shared_ptr<SummaryModel> model_;
  Grid* grid_ = nullptr;

  CommandRegistry command_registry_;
};
