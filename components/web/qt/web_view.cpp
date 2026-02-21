#include "components/web/qt/web_view.h"

#include "base/utf_convert.h"
#include "base/win/scoped_bstr.h"
#include "base/win/scoped_variant.h"
#include "controller/controller_registry.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_util.h"
#include "profile/window_definition.h"
#include "web/web_util.h"

#include <atlcomcli.h>

#include <ExDisp.h>
#include <QAxWidget>
#include <QUuid>
#include <wrl/client.h>

WebView::WebView(const ControllerContext& context)
    : ControllerContext{context} {}

WebView::~WebView() {}

std::unique_ptr<UiView> WebView::Init(const WindowDefinition& definition) {
  path_ = definition.path;

  auto url = IsWebUrl(path_.u16string())
                 ? path_.u16string()
                 : MakeFileUrl(GetPublicFilePath(path_));

  auto ax_widget = std::make_unique<QAxWidget>();
  ax_widget->setFocusPolicy(Qt::StrongFocus);
  ax_widget->setControl("{8856F961-340A-11D0-A96B-00C04FD705A2}");

  Microsoft::WRL::ComPtr<IWebBrowser2> web;
  ax_widget->queryInterface(IID_PPV_ARGS(&web));
  if (web) {
    web->put_Silent(TRUE);

    if (!url.empty()) {
      base::win::ScopedVariant e;
      VARIANT* empty = const_cast<VARIANT*>(e.ptr());
      web->Navigate(base::win::ScopedBstr{UtfConvert<wchar_t>(url)}, empty, empty,
                    empty, empty);
    }
  }

  ax_widget_ = ax_widget.get();
  return ax_widget;
}

void WebView::Save(WindowDefinition& definition) {
  definition.path = path_;
}
