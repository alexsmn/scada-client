#pragma once

#include "base/memory/weak_ptr.h"
#include "command_registry.h"
#include "controller.h"
#include "controller_context.h"
#include "selection_model.h"

#include <filesystem>
#include <memory>

class ModusView;
class ModusView2;
class ModusView3;
class ModusViewWrapper;

class ModusController : protected ControllerContext, public Controller {
 public:
  explicit ModusController(const ControllerContext& context);
  virtual ~ModusController();

  // Controller overrides
  virtual QWidget* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }

 private:
  QWidget* CreateModusView();
  QWidget* CreateModusView2();
  QWidget* CreateModusView3();

  void OpenPath(const std::filesystem::path& path);
  void OpenHyperlink(std::u16string_view hyperlink);

  SelectionModel selection_{{timed_data_service_}};

  ModusView* view_ = nullptr;
  ModusView2* view2_ = nullptr;
  ModusView3* view3_ = nullptr;
  ModusViewWrapper* wrapper_ = nullptr;

  CommandRegistry command_registry_;

  base::WeakPtrFactory<ModusController> weak_factory_{this};
};
