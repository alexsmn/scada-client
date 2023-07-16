#pragma once

#include "controller.h"
#include "selection_model.h"

#include <filesystem>

class ControllerDelegate;
class QWidget;
class TimedDataService;

namespace vidicon {
class VidiconClient;
}

struct VidiconDisplayView2Context {
  TimedDataService& timed_data_service_;
  vidicon::VidiconClient& vidicon_client_;
  ControllerDelegate& controller_delegate_;
};

class VidiconDisplayView2 : private VidiconDisplayView2Context,
                            public Controller {
 public:
  explicit VidiconDisplayView2(VidiconDisplayView2Context&& context);
  virtual ~VidiconDisplayView2();

  // Controller
  virtual QWidget* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }

 private:
  std::filesystem::path path_;

  QWidget* widget_ = nullptr;

  SelectionModel selection_;
};
