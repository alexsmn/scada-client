#pragma once

#include "components/modus/activex/modus_document.h"
#include "components/modus/modus_view_wrapper.h"
#include "common/node_state.h"
#include "ui/views/controls/activex_control.h"

#include <QAxWidget>

namespace base {
class FilePath;
}

class ModusController;

class ModusView : public QWidget,
                  public ModusViewWrapper,
                  private modus::ModusDocumentContext {
  Q_OBJECT

 public:
  explicit ModusView(modus::ModusDocumentContext&& context);
  virtual ~ModusView();

  // ModusViewWrapper
  virtual void Open(const std::filesystem::path& path) override;
  virtual std::filesystem::path GetPath() const override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;
  virtual htsde2::IHTSDEForm2* GetSdeForm() override;

 protected slots:
  void OnDocClick(IDispatch*, IDispatch*);
  void OnDocRightClick(IDispatch*, IDispatch*);
  void OnDocDblClick(IDispatch*, IDispatch*);
  void OnDocPopup(IDispatch*, bool&);

 protected:
  friend class ModusController;

  std::filesystem::path path_;

  // WARNING: QAxWidget must be a child of the view widget. Otherwise, it's not
  // responsive.
  QAxWidget* ax_widget_ = nullptr;

  std::unique_ptr<modus::ModusDocument> document_;
};
