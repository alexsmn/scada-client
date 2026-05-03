#pragma once

#include "controller/action_manager.h"
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
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual ActionManager* GetActionManager() override;
  virtual ContentsModel* GetContentsModel() override;

 private:
  void DeleteSelection();

  const std::shared_ptr<TransmissionModel> model_;
  const std::shared_ptr<aui::ColumnHeaderModel> column_model_;

  aui::Grid* grid_ = nullptr;

  ActionManager command_registry_;
};
