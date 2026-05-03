#pragma once

#include "controller/action_manager.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/selection_model.h"

#include <memory>

namespace aui {
class Grid;
}

class ExportModel;
class SummaryModel;

class SummaryView : protected ControllerContext, public Controller {
 public:
  explicit SummaryView(const ControllerContext& context);

  // Controller
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual ActionManager* GetActionManager() override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }
  virtual ContentsModel* GetContentsModel() override;
  virtual TimeModel* GetTimeModel() override;
  virtual ExportModel* GetExportModel() override;
  virtual std::optional<OpenContext> GetOpenContext() const override;

 private:
  SelectionModel selection_{{timed_data_service_}};

  const std::shared_ptr<SummaryModel> model_;
  aui::Grid* grid_ = nullptr;

  ActionManager command_registry_;
};
