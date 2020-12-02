#pragma once

#include "base/files/file_path.h"
#include "base/timer/timer.h"
#include "components/vidicon_display/displayviewerx.h"
#include "controller.h"
#include "controller_context.h"
#include "ui/views/controls/activex_control.h"

#include <wrl/client.h>

class VidiconDisplayView : protected ControllerContext,
                           public Controller,
                           private views::ActiveXControl::Controller {
 public:
  explicit VidiconDisplayView(const ControllerContext& context);
  virtual ~VidiconDisplayView();

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;

 private:
  void SynchronizeView();

  // views::ActiveXControl::Controller
  virtual void OnControlCreated(views::ActiveXControl& sender) override;
  virtual void OnContractDestroyed(views::ActiveXControl& sender) override;

  base::FilePath path_;

  std::unique_ptr<views::ActiveXControl> control_;
  Microsoft::WRL::ComPtr<ViewerX::IViewerForm> form_;

  base::Timer synchronize_timer_;
};
