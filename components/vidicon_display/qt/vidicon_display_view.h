#pragma once

#include "base/files/file_path.h"
#include "base/timer/timer.h"
#include "components/vidicon_display/display_viewer_api.h"
#include "controller.h"

#include <wrl/client.h>

class QAxWidget;

class VidiconDisplayView : public Controller {
 public:
  VidiconDisplayView();
  virtual ~VidiconDisplayView();

  // Controller
  virtual QWidget* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;

 private:
  void SynchronizeView();

  base::FilePath path_;

  std::unique_ptr<QAxWidget> ax_widget_;
  Microsoft::WRL::ComPtr<ViewerX::IViewerForm> form_;

  base::Timer synchronize_timer_;
};
