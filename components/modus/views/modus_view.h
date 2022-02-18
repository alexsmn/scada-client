#pragma once

#include "components/modus/activex/modus_document.h"
#include "components/modus/modus_view_wrapper.h"
#include "core/configuration_types.h"
#include "ui/views/controls/activex_control.h"

namespace base {
class FilePath;
}

class ModusController;

class ModusView : private modus::ModusDocumentContext,
                  public views::ActiveXControl,
                  public ModusViewWrapper,
                  private views::ActiveXControl::Controller {
 public:
  explicit ModusView(modus::ModusDocumentContext&& context);
  virtual ~ModusView();

  // ModusViewWrapper
  virtual void Open(const std::filesystem::path& path) override;
  virtual std::filesystem::path GetPath() const override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;
  virtual htsde2::IHTSDEForm2* GetSdeForm() override;

 protected:
  friend class ModusController;

  // views::ActiveXControl::Controller
  virtual void OnControlCreated(views::ActiveXControl& sender) override;
  virtual void OnContractDestroyed(views::ActiveXControl& sender) override;

  std::filesystem::path path_;

  std::unique_ptr<modus::ModusDocument> document_;
};
