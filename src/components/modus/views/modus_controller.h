#pragma once

#include <memory>

#include "base/memory/weak_ptr.h"
#include "controller.h"

namespace base {
class FilePath;
}

class ModusView;
class ModusView2;
class ModusViewWrapper;

class ModusController : public Controller {
 public:
  explicit ModusController(const ControllerContext& context);
  virtual ~ModusController();

  // Controller overrides
  virtual views::View* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual void ExecuteCommand(unsigned command) override;

 private:
  views::View* CreateModusView();
  views::View* CreateModusView2();

  void OpenPath(const base::FilePath& path);

  std::unique_ptr<ModusView> view_;
  std::unique_ptr<ModusView2> view2_;
  ModusViewWrapper* wrapper_ = nullptr;

  base::WeakPtrFactory<ModusController> weak_factory_;
};
