#pragma once

#include "base/cancelation.h"
#include "controller/command_registry.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/selection_model.h"

#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>

class ModusViewWrapper;

// Hosts a Modus display: builds the runtime view, opens the document named by
// the window definition, and binds what the operator clicks in the drawing to
// the client's live-data selection.
class ModusController : protected ControllerContext, public Controller {
 public:
  // The runtime view this controller embeds: the widget that goes into the
  // window, and the wrapper interface the controller drives. Production builds
  // one object that is both (`ModusVdsRuntimeView`, which is a
  // `VdsRuntimeWidget`), but they are kept as two pointers so a test can supply
  // a wrapper with no VDS runtime — and so no `tc_vds_runtime` shared library —
  // behind it. The widget is owned by the returned `UiView`; the wrapper is
  // owned by the widget tree.
  struct RuntimeView {
    QWidget* widget = nullptr;
    ModusViewWrapper* wrapper = nullptr;
  };

  // Builds the runtime view for a controller. An empty factory means the
  // production one; a test passes its own to drive the controller headlessly.
  using RuntimeViewFactory = std::function<RuntimeView(ModusController&)>;

  explicit ModusController(const ControllerContext& context,
                           RuntimeViewFactory runtime_view_factory = {});
  virtual ~ModusController();

  // Selection sink for the runtime view. Maps a VDS data-source string to a
  // live TimedData selection. An empty string means "the click hit nothing"
  // and leaves the selection alone — note that this is NOT the same as
  // clicking empty space, which the runtime does not report at all.
  //
  // Every other string selects something: `scada::NodeId::FromString` does not
  // fail, it falls through to a *string* NodeId, so a drawing that names a
  // stale or misspelled data source selects a node that does not resolve
  // rather than clearing. The clear below is reached only if building the
  // `TimedDataSpec` throws, which is why it is a catch and not a parse check.
  void SelectDataSource(std::string_view data_source);

  // Acknowledges the selected item's alarm — the runtime view's double click.
  void AcknowledgeSelection();

  // Controller overrides
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }

 private:
  RuntimeView CreateVdsRuntimeView();

  SelectionModel selection_{{timed_data_service_}};

  RuntimeViewFactory runtime_view_factory_;

  ModusViewWrapper* wrapper_ = nullptr;

  CommandRegistry command_registry_;

  Cancelation cancelation_;
};
