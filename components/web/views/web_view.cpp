#include "components/web/views/web_view.h"

#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/stringprintf.h"
#include "base/win/scoped_bstr.h"
#include "base/win/scoped_comptr.h"
#include "base/win/scoped_variant.h"
#include "client_paths.h"
#include "controller_factory.h"
#include "window_definition.h"
#include "views/client_application_views.h"

#include <exdisp.h>

REGISTER_CONTROLLER(WebView, ID_WEB_VIEW);

WebView::WebView(const ControllerContext& context)
    : ActiveXControl{*g_application_views},
      Controller{context} {
}

views::View* WebView::Init(const WindowDefinition& definition) {
  url_ = definition.path.value();
  return this;
}

void WebView::Save(WindowDefinition& definition) {
  definition.path = base::FilePath(url_);
}

void WebView::NativeControlCreated(HWND window_handle) {
  __super::NativeControlCreated(window_handle);

  if (url_.empty()) {
    base::FilePath path;
    PathService::Get(client::DIR_DOCUMENTATION, &path);
    path = path.Append(L"manual.txt");
    url_ = L"file://" + path.value();
  }

  CreateControl(OLESTR("{8856F961-340A-11D0-A96B-00C04FD705A2}"));

  base::win::ScopedComPtr<IWebBrowser2> web;
  QueryControl(IID_IWebBrowser2, web.ReceiveVoid());
  if (web) {
    web->put_Silent(TRUE);
    //web->put_Offline(TRUE);
    base::win::ScopedVariant e;
    VARIANT* empty = const_cast<VARIANT*>(e.ptr());
    web->Navigate(base::win::ScopedBstr(url_.c_str()), empty, empty, empty, empty);
  }
}
