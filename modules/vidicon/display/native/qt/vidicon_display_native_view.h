#pragma once

#include "vidicon/vidicon_compat.h"

#include "controller/controller.h"
#include "controller/selection_model.h"

#include <QString>
#include <QVariant>
#include <filesystem>

class ControllerDelegate;
class DialogService;
class DisplayFrame;
class NodeEventProvider;
class NodeService;
class QWidget;
class TimedDataService;
class WriteService;

namespace scada::vidicon {
class VidiconClient;
}

struct VidiconDisplayNativeViewContext {
  TimedDataService& timed_data_service_;
  ControllerDelegate& controller_delegate_;
  DialogService& dialog_service_;
  WriteService& write_service_;
  // For the reshelled display frame's bay strips.
  NodeEventProvider& node_event_provider_;
  NodeService& node_service_;
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

  std::filesystem::path path_;

  QWidget* widget_ = nullptr;

  // The reshell frame around the renderer when the UX theme is active (else
  // null — the bare renderer is returned). Used to push selected signals into
  // the Measurements strip.
  DisplayFrame* frame_ = nullptr;

  SelectionModel selection_;
};
