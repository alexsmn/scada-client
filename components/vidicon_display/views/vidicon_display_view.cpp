#include "components/vidicon_display/views/vidicon_display_view.h"

#include "base/win/scoped_bstr.h"
#include "client_utils.h"
#include "components/vidicon_display/teleclient.h"
#include "components/vidicon_display/telecontrolview.h"
#include "components/vidicon_display/vidicon_client.h"
#include "controller_factory.h"
#include "views/activex_host.h"
#include "views/ambient_props.h"
#include "window_definition.h"

#include <wrl/client.h>

//#import "c:\Program Files\Telecontrol\Vidicon\Bin\\TelecontrolView.tlb"
// raw_interfaces_only #import "c:\Program
// Files\Telecontrol\Vidicon\Bin\\TeleClient.dll" raw_interfaces_only #import
//"c:\Program Files\Telecontrol\Vidicon\Bin\\DisplayViewerX.ocx"
// raw_interfaces_only

namespace {

/*HRESULT AxPropPut(IDispatch& dispatch, LPCOLESTR name, VARIANT* val,
EXCEPINFO* excep = NULL) { DISPID dispid = 0; HRESULT res =
dispatch.GetIDsOfNames(IID_NULL, const_cast<LPOLESTR*>(&name), 1,
                                       LOCALE_SYSTEM_DEFAULT, &dispid);
        if (FAILED(res))
                return res;

        DISPID dispidPut = DISPID_PROPERTYPUT;
        DISPPARAMS params = { 0 };
        params.cArgs = 1;
        params.cNamedArgs = 1;
        params.rgdispidNamedArgs = &dispidPut;
        params.rgvarg = val;

        return dispatch.Invoke(dispid, IID_NULL, LOCALE_SYSTEM_DEFAULT,
                         DISPATCH_PROPERTYPUT, &params, NULL, excep, NULL);
}*/

}  // namespace

VidiconDisplayView::VidiconDisplayView(const ControllerContext& context)
    : ::Controller{context},
      control_{
          std::make_unique<views::ActiveXControl>(ActiveXHost::instance())},
      synchronize_timer_(false, true) {
  control_->set_controller(this);
}

VidiconDisplayView::~VidiconDisplayView() {}

views::View* VidiconDisplayView::Init(const WindowDefinition& definition) {
  path_ = definition.path;

  return control_.get();
}

void VidiconDisplayView::Save(WindowDefinition& definition) {
  definition.path = path_;
}

void VidiconDisplayView::SynchronizeView() {
  HWND form_window = ::GetWindow(control_->GetWindowHandle(), GW_CHILD);
  if (::IsWindow(form_window))
    ::PostMessage(form_window, WM_ENTERIDLE, 0, 0);
}

void VidiconDisplayView::OnControlCreated(views::ActiveXControl& sender) {
  // set ambient properties
  {
    Microsoft::WRL::ComPtr<IAxWinAmbientDispatchEx> ambientEx;
    control_->QueryHost(IID_PPV_ARGS(&ambientEx));
    assert(ambientEx);
    CComObject<AmbientProps>* ambient = NULL;
    CComObject<AmbientProps>::CreateInstance(&ambient);
    assert(ambient);
    ambient->display_name = L"Схема";
    ambientEx->SetAmbientDispatch(ambient);
    ambientEx->put_MessageReflect(ATL_VARIANT_TRUE);
  }

  LPOLESTR ole_class;
  if (SUCCEEDED(StringFromCLSID(__uuidof(ViewerX::ViewerForm), &ole_class))) {
    control_->CreateControl(ole_class);
    CoTaskMemFree(ole_class);
  }

  control_->QueryControl(IID_PPV_ARGS(&form_));
  if (form_) {
    // DispEventAdvise(form);
    // form->put_StatusVisible(VARIANT_FALSE);
    // form->put_ToolbarVisible(VARIANT_FALSE);
    // form->put_PagesVisible(SDECore::txPagesHidden);
    // form->put_AxBorderStyle(htsde2::afbNone);

    // TODO: Extract method, log all possible errors.
    Microsoft::WRL::ComPtr<TelecontrolView::ITelecontrolView> view;
    control_->QueryControl(IID_PPV_ARGS(&view));
    if (view) {
      VidiconClient::TeleClient* teleclient =
          VidiconClient::GetInstance().GetTeleClient();
      if (teleclient) {
        HRESULT res = view->SetClient(teleclient);
        DCHECK(SUCCEEDED(res));
      }
    }

    form_->put_AutoStartRuntime(VARIANT_TRUE);
    form_->put_AxBorderStyle(ViewerX::afbNone);

    base::FilePath full_path = GetPublicFilePath(path_);
    form_->put_FileName(base::win::ScopedBstr(full_path.value().c_str()));

    synchronize_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(10),
                             base::Bind(&VidiconDisplayView::SynchronizeView,
                                        base::Unretained(this)));
  }
}

void VidiconDisplayView::OnContractDestroyed(views::ActiveXControl& sender) {
  synchronize_timer_.Reset();
  form_.Reset();
}
