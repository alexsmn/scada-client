#pragma once

#include "controller.h"

#include <filesystem>

class ControllerDelegate;
class QWidget;

namespace vidicon {
class VidiconClient;
}

class VidiconDisplayView2 : public Controller {
 public:
  VidiconDisplayView2(vidicon::VidiconClient& vidicon_client,
                      ControllerDelegate& controller_delegate);
  virtual ~VidiconDisplayView2();

  // Controller
  virtual QWidget* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;

 private:
  vidicon::VidiconClient& vidicon_client_;
  ControllerDelegate& controller_delegate_;

  std::filesystem::path path_;

  QWidget* widget_ = nullptr;
};
