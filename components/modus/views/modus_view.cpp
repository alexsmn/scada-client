#include "components/modus/views/modus_view.h"

#include "components/modus/activex/modus.h"
#include "views/activex_host.h"
#include "views/ambient_props.h"

#include <wrl/client.h>

// ModusView

ModusView::ModusView(ModusDocumentContext&& context)
    : ModusDocumentContext{std::move(context)},
      views::ActiveXControl{ActiveXHost::instance()} {
  set_controller(this);
}

ModusView::~ModusView() {}

void ModusView::Open(const std::filesystem::path& path) {
  path_ = path;
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

void ModusView::OnControlCreated(views::ActiveXControl& sender) {
  assert(!document_);

  // set ambient properties
  {
    Microsoft::WRL::ComPtr<IAxWinAmbientDispatchEx> ambientEx;
    QueryHost(IID_PPV_ARGS(&ambientEx));
    assert(ambientEx);
    CComObject<AmbientProps>* ambient = NULL;
    CComObject<AmbientProps>::CreateInstance(&ambient);
    assert(ambient);
    ambient->display_name = OLESTR("Схема");
    ambientEx->SetAmbientDispatch(ambient);
    ambientEx->put_MessageReflect(ATL_VARIANT_TRUE);
  }

  if (!CreateControl(L"{001F373C-29D3-5C7E-A000-A0FC803D82EE}"))
    CreateControl(L"{001F373C-29D3-5F7E-A000-A0FC803D82EE}");

  Microsoft::WRL::ComPtr<htsde2::IHTSDEForm2> sde_form;
  QueryControl(IID_PPV_ARGS(&sde_form));
  if (!sde_form)
    return;

  document_ = std::make_unique<modus::ModusDocument>(
      ModusDocumentContext{*this}, *sde_form.Get(), path_);
  title_callback_(document_->title());
}

void ModusView::OnContractDestroyed(views::ActiveXControl& sender) {
  document_.reset();
}
