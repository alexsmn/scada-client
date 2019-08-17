#pragma once

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "command_handler.h"
#include "controller.h"

#include <memory>

class ModusView;
class ModusView2;
class ModusViewWrapper;

class ModusController : public Controller, public CommandHandler {
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

  void OpenHyperlink(base::StringPiece16 hyperlink);
  void OpenPath(const base::FilePath& path);

  void ShowPopupMenu(const gfx::Point& point);

  std::unique_ptr<ModusView> view_;
  std::unique_ptr<ModusView2> view2_;
  ModusViewWrapper* wrapper_;

  base::WeakPtrFactory<ModusController> weak_factory_{this};
};
