#pragma once

#include "components/modus/activex/modus_document.h"
#include "components/modus/modus_view_wrapper.h"

#include <QWidget>
#include <filesystem>
#include <stop_token>

class QAxWidget;

// Native Modus ActiveXeme viewer.
class ModusView : public QWidget,
                  public ModusViewWrapper,
                  private modus::ModusDocumentContext {
  Q_OBJECT

 public:
  explicit ModusView(modus::ModusDocumentContext&& context);
  virtual ~ModusView();

  bool IsToolbarVisible() const;
  void SetToolbarVisible(bool visible);

  void ShowSetupDialog();

  // ModusViewWrapper
  virtual void Open(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual std::filesystem::path GetPath() const override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;

 protected slots:
  void OnDocClick(IDispatch*, IDispatch*);
  void OnDocRightClick(IDispatch*, IDispatch*);
  void OnDocDblClick(IDispatch*, IDispatch*);
  void OnDocPopup(IDispatch*, bool&);

 protected:
  void OpenPlaceholder();

  void DelayedOpen(const WindowDefinition& definition,
                   const std::stop_token& cancelation);

  std::filesystem::path path_;

  // WARNING: QAxWidget must be a child of the view widget. Otherwise, it's not
  // responsive.
  QAxWidget* ax_widget_ = nullptr;

  std::unique_ptr<modus::ModusDocument> document_;

  std::stop_source cancelation_;
};
