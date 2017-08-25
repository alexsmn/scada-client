#pragma once

#include "base/files/file_path.h"
#include "base/timer/timer.h"
#include "base/win/scoped_comptr.h"
#include "client/controller.h"
#include "client/components/vidicon_display/views/displayviewerx.h"
#include "ui/views/controls/activex_control.h"

class VidiconClient;

class VidiconDisplayView : public Controller,
                           private views::ActiveXControl::Controller {
 public:
  explicit VidiconDisplayView(const ControllerContext& context);
  virtual ~VidiconDisplayView();

  // Controller
  virtual views::View* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;

 private:
  void SynchronizeView();

  // views::ActiveXControl::Controller
  virtual void OnControlCreated(views::ActiveXControl& sender) override;
  virtual void OnContractDestroyed(views::ActiveXControl& sender) override;

  base::FilePath path_;

  std::unique_ptr<views::ActiveXControl> control_;
  base::win::ScopedComPtr<ViewerX::IViewerForm> form_;

  base::Timer synchronize_timer_;
};
