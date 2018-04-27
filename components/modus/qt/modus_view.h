#pragma once

#include "components/modus/activex/modus_document.h"
#include "components/modus/modus_view_wrapper.h"
#include "core/configuration_types.h"
#include "ui/views/controls/activex_control.h"

#include <QAxWidget>

namespace base {
class FilePath;
}

class ModusController;

class ModusView : private modus::ModusDocumentContext,
                  public QAxWidget,
                  public ModusViewWrapper {
 public:
  explicit ModusView(modus::ModusDocumentContext&& context);
  virtual ~ModusView();

  // ModusViewWrapper
  virtual void Open(const base::FilePath& path) override;
  virtual base::FilePath GetPath() const override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;
  virtual htsde2::IHTSDEForm2* GetSdeForm() override;

 protected:
  friend class ModusController;

  base::FilePath path_;

  std::unique_ptr<modus::ModusDocument> document_;
};
