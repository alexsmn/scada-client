#pragma once

#include "base/cancelation.h"
#include "common/aliases.h"
#include "controller/command_registry.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/selection_model.h"

#include <filesystem>
#include <memory>

class ModusView;
class ModusView2;
class ModusView3;
class ModusViewWrapper;

class ModusController : protected ControllerContext, public Controller {
 public:
  ModusController(const ControllerContext& context,
                  AliasResolver alias_resolver);
  virtual ~ModusController();

  // Controller overrides
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
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

  const AliasResolver alias_resolver_;

  SelectionModel selection_{{timed_data_service_}};

  ModusView* view_ = nullptr;
  ModusView2* view2_ = nullptr;
  ModusView3* view3_ = nullptr;
  ModusViewWrapper* wrapper_ = nullptr;

  CommandRegistry command_registry_;

  Cancelation cancelation_;
};
