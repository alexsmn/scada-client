#include "vidicon/display/activex/qt/vidicon_display_activex_view.h"

#include <cassert>

#include "base/win/scoped_bstr.h"
#include "filesystem/file_util.h"
#include "profile/window_definition.h"
#include "vidicon/display/activex/telecontrolview.h"
#include "vidicon/teleclient/vidicon_client.h"

#include <QAxWidget>
#include <QUuid>
#include <TeleClient.h>
#include <wrl/client.h>

// #import "c:\Program Files\Telecontrol\Vidicon\Bin\\TelecontrolView.tlb"
//  raw_interfaces_only #import "c:\Program
//  Files\Telecontrol\Vidicon\Bin\\TeleClient.dll" raw_interfaces_only #import
//"c:\Program Files\Telecontrol\Vidicon\Bin\\DisplayViewerX.ocx"
//  raw_interfaces_only

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

VidiconDisplayActiveXView::VidiconDisplayActiveXView(
    vidicon::VidiconClient& vidicon_client)
    : vidicon_client_{vidicon_client}, synchronize_timer_(false, true) {}

VidiconDisplayActiveXView::~VidiconDisplayActiveXView() {}

std::unique_ptr<UiView> VidiconDisplayActiveXView::Init(
    const WindowDefinition& definition) {
  path_ = definition.path;

  auto ax_widget = std::make_unique<QAxWidget>();
  ax_widget->disableMetaObject();
  ax_widget->disableClassInfo();
  ax_widget->disableEventSink();

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
      HRESULT res = view->SetClient(&vidicon_client_.teleclient());
      assert(SUCCEEDED(res));
    }

    form_->put_AutoStartRuntime(VARIANT_TRUE);
    form_->put_AxBorderStyle(ViewerX::afbNone);

    auto full_path = GetPublicFilePath(path_);
    form_->put_FileName(base::win::ScopedBstr(full_path.wstring()));

    /*synchronize_timer_.Start(
        FROM_HERE, base::TimeDelta::FromMilliseconds(10),
                             base::Bind(&VidiconDisplayActiveXView::SynchronizeView,
                                        base::Unretained(this)));*/
  }

  ax_widget_ = ax_widget.get();
  return ax_widget;
}

void VidiconDisplayActiveXView::Save(WindowDefinition& definition) {
  definition.path = path_;
}

void VidiconDisplayActiveXView::SynchronizeView() {
  // TODO.
  HWND form_window = ::GetWindow((HWND)(ax_widget_->winId()), GW_CHILD);
  if (::IsWindow(form_window))
    ::PostMessage(form_window, WM_ENTERIDLE, 0, 0);
}
