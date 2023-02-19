#include "components/vidicon_display/qt/vidicon_display_view.h"

#include "base/win/scoped_bstr.h"
#include "client_utils.h"
#include "components/vidicon_display/teleclient.h"
#include "components/vidicon_display/telecontrolview.h"
#include "components/vidicon_display/vidicon_client.h"
#include "views/ambient_props.h"
#include "window_definition.h"

#include <QAxWidget>
#include <QUuid>
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

VidiconDisplayView::VidiconDisplayView() : synchronize_timer_(false, true) {}

VidiconDisplayView::~VidiconDisplayView() {}

UiView* VidiconDisplayView::Init(const WindowDefinition& definition) {
  path_ = definition.path;

  auto ax_widget = std::make_unique<QAxWidget>();

  LPOLESTR ole_class;
  if (SUCCEEDED(StringFromCLSID(__uuidof(ViewerX::ViewerForm), &ole_class))) {
    ax_widget->setControl(QString::fromWCharArray(ole_class));
    CoTaskMemFree(ole_class);
  }

  ax_widget->queryInterface(IID_PPV_ARGS(&form_));
  if (form_) {
    // DispEventAdvise(form);
    // form->put_StatusVisible(VARIANT_FALSE);
    // form->put_ToolbarVisible(VARIANT_FALSE);
    // form->put_PagesVisible(SDECore::txPagesHidden);
    // form->put_AxBorderStyle(htsde2::afbNone);

    // TODO: Extract method, log all possible errors.
    Microsoft::WRL::ComPtr<TelecontrolView::ITelecontrolView> view;
    ax_widget->queryInterface(IID_PPV_ARGS(&view));
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

    std::filesystem::path full_path = GetPublicFilePath(path_);
    form_->put_FileName(base::win::ScopedBstr(full_path.wstring()));

    /*synchronize_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(10),
                             base::Bind(&VidiconDisplayView::SynchronizeView,
                                        base::Unretained(this)));*/
  }

  ax_widget_ = ax_widget.get();
  return ax_widget.release();
}

void VidiconDisplayView::Save(WindowDefinition& definition) {
  definition.path = path_;
}

void VidiconDisplayView::SynchronizeView() {
  // TODO.
  // HWND form_window = ::GetWindow((HWND)(ax_widget_->winId()), GW_CHILD);
  // if (::IsWindow(form_window))
  //  ::PostMessage(form_window, WM_ENTERIDLE, 0, 0);
}
