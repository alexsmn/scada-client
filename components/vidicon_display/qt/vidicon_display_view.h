#pragma once

#include "base/timer/timer.h"
#include "components/vidicon_display/display_viewer_api.h"
#include "controller/controller.h"

#include <filesystem>
#include <wrl/client.h>

namespace vidicon {
class VidiconClient;
}

class QAxWidget;

class VidiconDisplayView : public Controller {
 public:
  explicit VidiconDisplayView(vidicon::VidiconClient& vidicon_client);
  virtual ~VidiconDisplayView();

  // Controller
  virtual QWidget* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;

 private:
  void SynchronizeView();

  vidicon::VidiconClient& vidicon_client_;

  std::filesystem::path path_;

  QAxWidget* ax_widget_ = nullptr;
  Microsoft::WRL::ComPtr<ViewerX::IViewerForm> form_;

  base::Timer synchronize_timer_;
};
