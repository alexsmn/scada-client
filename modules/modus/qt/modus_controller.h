#pragma once

#include "base/cancelation.h"
#include "controller/command_registry.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/selection_model.h"

#include <filesystem>
#include <memory>

class ModusViewWrapper;

class ModusController : protected ControllerContext, public Controller {
 public:
  explicit ModusController(const ControllerContext& context);
  virtual ~ModusController();

  // Controller overrides
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }

 private:
  QWidget* CreateRuntimeView();

  SelectionModel selection_{{timed_data_service_}};

  ModusViewWrapper* wrapper_ = nullptr;

  CommandRegistry command_registry_;

  Cancelation cancelation_;
};
