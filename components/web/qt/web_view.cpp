#include "components/web/qt/web_view.h"

#include "base/strings/string_util.h"
#include "base/win/scoped_bstr.h"
#include "base/win/scoped_variant.h"
#include "controller/controller_registry.h"
#include "profile/window_definition.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_util.h"
#include "web/web_util.h"

#include <atlcomcli.h>

#include <ExDisp.h>
#include <QAxWidget>
#include <QUuid>
#include <wrl/client.h>

WebView::WebView(const ControllerContext& context)
    : ControllerContext{context} {}

WebView::~WebView() {}

UiView* WebView::Init(const WindowDefinition& definition) {
  path_ = definition.path;

  auto url = IsWebUrl(path_.u16string())
                 ? path_.u16string()
                 : MakeFileUrl(GetPublicFilePath(path_));

  ax_widget_ = new QAxWidget{};
  ax_widget_->setFocusPolicy(Qt::StrongFocus);
  ax_widget_->setControl("{8856F961-340A-11D0-A96B-00C04FD705A2}");

  Microsoft::WRL::ComPtr<IWebBrowser2> web;
  ax_widget_->queryInterface(IID_PPV_ARGS(&web));
  if (web) {
    web->put_Silent(TRUE);

    if (!url.empty()) {
      base::win::ScopedVariant e;
      VARIANT* empty = const_cast<VARIANT*>(e.ptr());
      web->Navigate(base::win::ScopedBstr{base::AsWString(url)}, empty, empty,
                    empty, empty);
    }
  }

  return ax_widget_;
}

void WebView::Save(WindowDefinition& definition) {
  definition.path = path_;
}
