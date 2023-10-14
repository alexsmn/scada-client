#pragma once

#include "controller/controller.h"
#include "controller/selection_model.h"

#include <filesystem>

class ControllerDelegate;
class QWidget;
class TimedDataService;

namespace vidicon {
class VidiconClient;
}

struct VidiconDisplayNativeViewContext {
  TimedDataService& timed_data_service_;
  vidicon::VidiconClient& vidicon_client_;
  ControllerDelegate& controller_delegate_;
};

class VidiconDisplayNativeView : private VidiconDisplayNativeViewContext,
                                 public Controller {
 public:
  explicit VidiconDisplayNativeView(VidiconDisplayNativeViewContext&& context);
  virtual ~VidiconDisplayNativeView();

  // Controller
  virtual QWidget* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }

 private:
  std::filesystem::path path_;

  QWidget* widget_ = nullptr;

  SelectionModel selection_;
};
