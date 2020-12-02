#include "components/web/qt/web_view.h"

#include "base/win/scoped_bstr.h"
#include "base/win/scoped_variant.h"
#include "client_utils.h"
#include "controller_factory.h"
#include "services/file_cache.h"
#include "window_definition.h"

#include <atlcomcli.h>

#include <Exdisp.h>
#include <QAxWidget>
#include <QUuid>
#include <wrl/client.h>

WebView::WebView(const ControllerContext& context)
    : ControllerContext{context} {}

WebView::~WebView() {}

UiView* WebView::Init(const WindowDefinition& definition) {
  path_ = definition.path;
  std::wstring url = IsWebUrl(definition.path.value())
                         ? definition.path.value()
                         : MakeFileUrl(GetPublicFilePath(definition.path));

  ax_widget_ = std::make_unique<QAxWidget>();
  ax_widget_->setFocusPolicy(Qt::StrongFocus);
  ax_widget_->setControl("{8856F961-340A-11D0-A96B-00C04FD705A2}");

  Microsoft::WRL::ComPtr<IWebBrowser2> web;
  ax_widget_->queryInterface(IID_PPV_ARGS(&web));
  if (web) {
    web->put_Silent(TRUE);

    if (!url.empty()) {
      base::win::ScopedVariant e;
      VARIANT* empty = const_cast<VARIANT*>(e.ptr());
      web->Navigate(base::win::ScopedBstr(url.c_str()), empty, empty, empty,
                    empty);
    }
  }

  return ax_widget_.get();
}

void WebView::Save(WindowDefinition& definition) {
  definition.path = path_;
}
