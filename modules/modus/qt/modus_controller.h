#pragma once

#include "base/cancelation.h"
#include "controller/command_registry.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/selection_model.h"
#include "filesystem/file_cache.h"

#include <QPointer>
#include <QWidget>

#include <filesystem>
#include <functional>
#include <memory>
#include <string_view>

class ModusLiveData;
class ModusViewWrapper;

// Hosts a Modus display: builds the display view, opens the document named by
// the window definition, and binds what the operator clicks in the drawing to
// the client's live-data selection.
class ModusController : protected ControllerContext, public Controller {
 public:
  // The display view this controller embeds: the widget that goes into the
  // window, and the wrapper interface the controller drives. Production builds
  // one object that is both (`ModusDisplayView`, which is a `DisplayWidget`),
  // but they are kept as two pointers so a test can supply a wrapper that
  // opens no document at all. The widget is owned by the returned `UiView`;
  // the wrapper is owned by the widget tree.
  struct DisplayView {
    QWidget* widget = nullptr;
    ModusViewWrapper* wrapper = nullptr;
  };

  // Builds the display view for a controller. An empty factory means the
  // production one; a test passes its own to drive the controller headlessly.
  using DisplayViewFactory = std::function<DisplayView(ModusController&)>;

  explicit ModusController(const ControllerContext& context,
                           DisplayViewFactory display_view_factory = {});
  virtual ~ModusController();

  // Selection sink for the display view. Maps an element's data source — the
  // formula from its `ключ_привязки` — to a live TimedData selection,
  // resolved by the formula service exactly as the schematic's own
  // subscription is, so an alias selects the object it names (backlog 915).
  // An empty string means "the click hit an element bound to nothing" and
  // clears the selection, as a click on bare page does (backlog 907): the
  // highlight has moved off the previous object, so the selection must too.
  //
  // A formula the service cannot resolve still selects something: it yields
  // an error TimedData rather than throwing, so a drawing that names a stale
  // or misspelled alias selects an object that shows as not found rather
  // than clearing. The clear below is reached only if building the
  // `TimedDataSpec` throws, which is why it is a catch and not a parse check.
  void SelectDataSource(std::string_view data_source);

  // Acknowledges the selected item's alarm — the display view's double click.
  void AcknowledgeSelection();

  // Controller overrides
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;
  virtual CommandHandler* GetCommandHandler(unsigned command_id) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }

 private:
  DisplayView CreateDisplayView();

  // Indexes the open schematic in the file cache under its title and the
  // objects it is bound to, when either changed since the last time.
  void UpdateFileCache();

  SelectionModel selection_{{timed_data_service_}};

  DisplayViewFactory display_view_factory_;

  ModusViewWrapper* wrapper_ = nullptr;

  // The widget the wrapper lives in, which the window owns. Null once the
  // window has destroyed it, so a late reading is dropped rather than
  // delivered to a deleted view.
  QPointer<QWidget> widget_;

  // The schematic's live bindings, from `Init` on (backlog 915).
  std::unique_ptr<ModusLiveData> live_data_;

  // What `UpdateFileCache` last wrote, so an unchanged index is not rewritten
  // on every reading.
  std::u16string indexed_title_;
  FileCache::ItemMap indexed_items_;

  CommandRegistry command_registry_;

  Cancelation cancelation_;
};
