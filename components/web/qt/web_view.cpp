#include "components/web/qt/web_view.h"

#include "base/win/scoped_bstr.h"
#include "base/win/scoped_variant.h"
#include "controller_factory.h"
#include "window_definition.h"

#include <atlcomcli.h>

#include <Exdisp.h>
#include <wrl/client.h>
#include <QAxWidget>
#include <QUuid>

const WindowInfo kWindowInfo = {ID_WEB_VIEW, "Web", L"Web"};

REGISTER_CONTROLLER(WebView, kWindowInfo);

WebView::WebView(const ControllerContext& context) : Controller{context} {}

WebView::~WebView() {}

UiView* WebView::Init(const WindowDefinition& definition) {
  url_ = definition.path.value();

  ax_widget_ = std::make_unique<QAxWidget>();
  ax_widget_->setControl("{8856F961-340A-11D0-A96B-00C04FD705A2}");

  Microsoft::WRL::ComPtr<IWebBrowser2> web;
  ax_widget_->queryInterface(IID_PPV_ARGS(&web));
  if (web) {
    web->put_Silent(TRUE);

    if (!url_.empty()) {
      base::win::ScopedVariant e;
      VARIANT* empty = const_cast<VARIANT*>(e.ptr());
      web->Navigate(base::win::ScopedBstr(url_.c_str()), empty, empty, empty,
                    empty);
    }
  }

  return ax_widget_.get();
}

void WebView::Save(WindowDefinition& definition) {
  definition.path = base::FilePath{url_};
}
