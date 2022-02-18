#include "components/modus/qt/modus_view.h"

#include "components/modus/activex/modus.h"

#include <QHBoxLayout>
#include <QUuid>
#include <atlcomcli.h>
#include <wrl/client.h>

ModusView::ModusView(ModusDocumentContext&& context)
    : ModusDocumentContext{std::move(context)} {
  auto* layout = new QHBoxLayout{this};
  layout->setContentsMargins(0, 0, 0, 0);
  setLayout(layout);

  ax_widget_ = new QAxWidget{this};
  layout->addWidget(ax_widget_);

  if (!ax_widget_->setControl("{001F373C-29D3-5C7E-A000-A0FC803D82EE}"))
    ax_widget_->setControl("{001F373C-29D3-5F7E-A000-A0FC803D82EE}");
}

ModusView::~ModusView() {}

void ModusView::Open(const std::filesystem::path& path) {
  assert(!document_);

  path_ = path;

  Microsoft::WRL::ComPtr<htsde2::IHTSDEForm2> sde_form;
  ax_widget_->queryInterface(IID_PPV_ARGS(&sde_form));
  if (!sde_form)
    return;

  document_ = std::make_unique<modus::ModusDocument>(
      ModusDocumentContext{*this}, *sde_form.Get(), path_);

  connect(ax_widget_, SIGNAL(OnDocClick(IDispatch*, IDispatch*)), this,
          SLOT(OnDocClick(IDispatch*, IDispatch*)));
  connect(ax_widget_, SIGNAL(OnDocRightClick(IDispatch*, IDispatch*)), this,
          SLOT(OnDocRightClick(IDispatch*, IDispatch*)));
  connect(ax_widget_, SIGNAL(OnDocDblClick(IDispatch*, IDispatch*)), this,
          SLOT(OnDocDblClick(IDispatch*, IDispatch*)));
  connect(ax_widget_, SIGNAL(OnDocPopup(IDispatch*, bool&)), this,
          SLOT(OnDocPopup(IDispatch*, bool&)));

  title_callback_(document_->title());
}

std::filesystem::path ModusView::GetPath() const {
  return path_;
}

bool ModusView::ShowContainedItem(const scada::NodeId& item_id) {
  return document_ && document_->ShowContainedItem(item_id);
}

htsde2::IHTSDEForm2* ModusView::GetSdeForm() {
  return document_ ? &document_->sde_form() : nullptr;
}

void ModusView::OnDocClick(IDispatch* disp_doc, IDispatch* disp_info) {
  CComQIPtr<SDECore::IUIEventInfo> ui_event_info(disp_info);

  if (!document_ || !ui_event_info)
    return;

  document_->OnDocClick(modus::ModusDocument::MouseButton::Left,
                        *ui_event_info);
}

void ModusView::OnDocRightClick(IDispatch* disp_doc, IDispatch* disp_info) {
  CComQIPtr<SDECore::IUIEventInfo> ui_event_info(disp_info);

  if (!document_ || !ui_event_info)
    return;

  document_->OnDocClick(modus::ModusDocument::MouseButton::Right,
                        *ui_event_info);
}

void ModusView::OnDocDblClick(IDispatch* disp_doc, IDispatch* disp_info) {
  CComQIPtr<SDECore::IUIEventInfo> ui_event_info(disp_info);

  if (!document_ || !ui_event_info)
    return;

  document_->OnDocDblClick(*ui_event_info);
}

void ModusView::OnDocPopup(IDispatch* disp_doc, bool& popup) {
  if (document_)
    document_->OnDocPopup(popup);
}
