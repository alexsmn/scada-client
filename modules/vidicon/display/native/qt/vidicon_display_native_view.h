#pragma once

#include "controller/controller.h"
#include "controller/selection_model.h"

#include <QPointer>
#include <QString>
#include <QVariant>
#include <boost/signals2/connection.hpp>
#include <filesystem>
#include <memory>
#include <string_view>

class BlinkerManager;
class ControllerDelegate;
class DisplayLiveData;
class DisplayWidget;
class DialogService;
class DisplayFrame;
class DisplaySelectionRegistry;
class NodeEventProvider;
class NodeService;
class QWidget;
class TimedDataService;
class WriteService;

namespace scada {
class SessionService;
}

namespace scada::vidicon {
class VidiconClient;
}

struct VidiconDisplayNativeViewContext {
  // An unparseable object address is reported through a message box, which is
  // a lazy awaitable and needs an executor to be spawned on. See
  // `aui/show_message_box.h`.
  const AnyExecutor executor_;
  TimedDataService& timed_data_service_;
  ControllerDelegate& controller_delegate_;
  DialogService& dialog_service_;
  WriteService& write_service_;
  // For the reshelled display frame's bay strips.
  NodeEventProvider& node_event_provider_;
  NodeService& node_service_;
  // Drives the display frame's Live pill (backlog 911).
  scada::SessionService& session_service_;
  // Where the selected element is published for the status strip's `Selected`
  // cell. The view cannot reach the status bar itself -- it is built once for
  // the window -- so the registry is the seam. ADR 0012 phase 5.
  DisplaySelectionRegistry& display_selection_registry_;
  // Blinks the elements whose objects have an unacknowledged alarm.
  BlinkerManager& blinker_manager_;
};

class VidiconDisplayNativeView : private VidiconDisplayNativeViewContext,
                                 public Controller {
 public:
  explicit VidiconDisplayNativeView(VidiconDisplayNativeViewContext&& context);
  virtual ~VidiconDisplayNativeView();

  // Controller
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }

 private:
  // TODO: Change to `scada::` types, use `scada::InvokeMethod`, extract
  // `VidiconDisplayCommandHandler` class, and move it to the upper level.
  void ExecCommand(const QString& command_name, const QVariantList& arguments);
  void OpenWriteWin(const QString& data_source, bool manual);

  // Selects the object a clicked element is bound to: a tag's address, or in
  // a display without tags a shape name naming a node.
  void SelectDataSource(std::string_view data_source);

  void OnBlink(bool state);

  std::filesystem::path path_;

  // The display frame around the renderer. Used to push selected signals into
  // the Measurements strip.
  DisplayFrame* frame_ = nullptr;

  SelectionModel selection_;

  // The renderer, owned by the frame. Null once the window destroyed it.
  QPointer<DisplayWidget> widget_;

  // The display's live bindings: every address its shapes' tags name.
  std::unique_ptr<DisplayLiveData> live_data_;

  // After `live_data_`, so it disconnects first.
  boost::signals2::scoped_connection blink_connection_;
};
