#pragma once

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "command_handler_impl.h"
#include "controller.h"
#include "controller_context.h"
#include "selection_model.h"

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

  void OpenPath(const base::FilePath& path);
  void OpenHyperlink(std::wstring_view hyperlink);

  SelectionModel selection_{{timed_data_service_}};

  std::unique_ptr<ModusView> view_;
  std::unique_ptr<ModusView2> view2_;
  std::unique_ptr<ModusView3> view3_;
  ModusViewWrapper* wrapper_;

  CommandHandlerImpl command_handler_;

  base::WeakPtrFactory<ModusController> weak_factory_{this};
};
