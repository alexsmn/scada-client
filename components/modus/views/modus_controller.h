#pragma once

#include "base/memory/weak_ptr.h"
#include "command_registry.h"
#include "controller.h"
#include "controller_context.h"
#include "controls/point.h"
#include "selection_model.h"

#include <filesystem>
#include <memory>

class ModusView;
class ModusView2;
class ModusViewWrapper;

class ModusController : protected ControllerContext, public Controller {
 public:
  explicit ModusController(const ControllerContext& context);
  virtual ~ModusController();

  // Controller overrides
  virtual views::View* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }

 private:
  views::View* CreateModusView();
  views::View* CreateModusView2();

  void OpenHyperlink(std::u16string_view hyperlink);
  void OpenPath(const std::filesystem::path& path);

  void ShowPopupMenu(const aui::Point& point);

  SelectionModel selection_{{timed_data_service_}};

  std::unique_ptr<ModusView> view_;
  std::unique_ptr<ModusView2> view2_;
  ModusViewWrapper* wrapper_;

  CommandRegistry command_registry_;

  base::WeakPtrFactory<ModusController> weak_factory_{this};
};
