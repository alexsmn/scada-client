#pragma once

#include "controller.h"

#include <filesystem>

class QWidget;

namespace vidicon {
class VidiconClient;
}

class VidiconDisplayView2 : public Controller {
 public:
  explicit VidiconDisplayView2(vidicon::VidiconClient& vidicon_client);
  virtual ~VidiconDisplayView2();

  // Controller
  virtual QWidget* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;

 private:
  vidicon::VidiconClient& vidicon_client_;

  std::filesystem::path path_;

  QWidget* widget_ = nullptr;
};
