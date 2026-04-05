#pragma once

#include "controller/controller.h"
#include "vidicon/display/activex/display_viewer_api.h"

#include <filesystem>
#include <wrl/client.h>

namespace vidicon {
class VidiconClient;
}

class QAxWidget;

class VidiconDisplayActiveXView : public Controller {
 public:
  explicit VidiconDisplayActiveXView(vidicon::VidiconClient& vidicon_client);
  virtual ~VidiconDisplayActiveXView();

  // Controller
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;

 private:
  void SynchronizeView();

  vidicon::VidiconClient& vidicon_client_;

  std::filesystem::path path_;

  QAxWidget* ax_widget_ = nullptr;
  Microsoft::WRL::ComPtr<ViewerX::IViewerForm> form_;

};
