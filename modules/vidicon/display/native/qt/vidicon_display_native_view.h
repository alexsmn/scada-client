#pragma once

#include "controller/controller.h"
#include "controller/selection_model.h"

#include <QString>
#include <QVariant>
#include <filesystem>

class ControllerDelegate;
class DialogService;
class QWidget;
class TimedDataService;
class WriteService;

namespace vidicon {
class VidiconClient;
}

struct VidiconDisplayNativeViewContext {
  TimedDataService& timed_data_service_;
  vidicon::VidiconClient& vidicon_client_;
  ControllerDelegate& controller_delegate_;
  DialogService& dialog_service_;
  WriteService& write_service_;
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

  SelectionModel selection_;
};
