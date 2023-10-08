#include "components/modus/qt/modus_view.h"

#include "base/bind.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "components/modus/activex/modus.h"
#include "filesystem/file_util.h"
#include "controller/window_definition.h"

#include <QAxWidget>
#include <QDesktopServices>
#include <QHBoxLayout>
#include <QLabel>
#include <QUrl>
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

  const std::string_view kClassIds[] = {
      "{001F373C-29D3-630E-A0A0-A0FC803D82EE}",
      "{001F373C-29D3-5C7E-A000-A0FC803D82EE}",
      "{001F373C-29D3-5F7E-A000-A0FC803D82EE}"};

  for (const auto& class_id : kClassIds) {
    if (ax_widget_->setControl(QString::fromLocal8Bit(
            class_id.data(), static_cast<int>(class_id.size())))) {
      break;
    }
  }
}

ModusView::~ModusView() = default;

bool ModusView::IsToolbarVisible() const {
  VARIANT_BOOL toolbar_visible = 0;
  if (document_) {
    document_->sde_form().get_ToolbarVisible(&toolbar_visible);
  }
  return toolbar_visible != 0;
}

void ModusView::SetToolbarVisible(bool visible) {
  if (document_) {
    document_->sde_form().put_ToolbarVisible(visible);
  }
}

void ModusView::Open(const WindowDefinition& definition) {
  assert(!document_);

  path_ = GetPublicFilePath(definition.path);

  Microsoft::WRL::ComPtr<htsde2::IHTSDEForm2> sde_form;
  ax_widget_->queryInterface(IID_PPV_ARGS(&sde_form));
  if (!sde_form) {
    OpenPlaceholder();
    return;
  }

  document_ = std::make_unique<modus::ModusDocument>(
      ModusDocumentContext{*this}, *sde_form.Get());

  document_->InitFromFilePath(path_);

  connect(ax_widget_, SIGNAL(OnDocClick(IDispatch*, IDispatch*)), this,
          SLOT(OnDocClick(IDispatch*, IDispatch*)));
  connect(ax_widget_, SIGNAL(OnDocRightClick(IDispatch*, IDispatch*)), this,
          SLOT(OnDocRightClick(IDispatch*, IDispatch*)));
  connect(ax_widget_, SIGNAL(OnDocDblClick(IDispatch*, IDispatch*)), this,
          SLOT(OnDocDblClick(IDispatch*, IDispatch*)));
  connect(ax_widget_, SIGNAL(OnDocPopup(IDispatch*, bool&)), this,
          SLOT(OnDocPopup(IDispatch*, bool&)));

  title_callback_(document_->title());

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&ModusView::DelayedOpen, base::Unretained(this),
                            definition, cancelation_.get_token()));
}

void ModusView::DelayedOpen(const WindowDefinition& definition,
                            const std::stop_token& cancelation) {
  if (cancelation.stop_requested()) {
    return;
  }

  if (auto* sde_document = document_->sde_document()) {
    Microsoft::WRL::ComPtr<SDECore::ISDEPages> pages;
    sde_document->get_Pages(pages.ReleaseAndGetAddressOf());
    for (const auto& item : definition.items) {
      if (item.name_is("Page")) {
        auto index = item.GetInt("index");
        auto scale = item.GetInt("scale") / 100.0;
        Microsoft::WRL::ComPtr<SDECore::ISDEPage50> page;
        pages->get_Item(CComVariant{index}, page.ReleaseAndGetAddressOf());
        if (page) {
          page->put_Scale(scale);
        }
      }
    }
  }
}

void ModusView::OpenPlaceholder() {
  delete ax_widget_;
  ax_widget_ = nullptr;

  QLabel* placeholder = new QLabel{this};
  placeholder->setTextFormat(Qt::RichText);
  placeholder->setText(
      QString::fromWCharArray(LR"(<html><body>
    <p>Отсутствует компонент Modus ActivesXeme, используемый для отображения схем Модус.</p>
    <p>Загрузите бесплатную версию компонента
      с <a href="https://swman.ru">сайта производителя</a> или включите
      экспериментальную <a href="#internal-render">встроенную отрисовку</a>.</p>
    </body></html>)"));
  placeholder->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  placeholder->setWordWrap(true);
  placeholder->setTextInteractionFlags(Qt::TextBrowserInteraction);
  connect(placeholder, &QLabel::linkActivated,
          [enable_internal_render_callback =
               enable_internal_render_callback_](const QString& link) {
            if (link == "#internal-render") {
              enable_internal_render_callback();
            } else {
              QDesktopServices::openUrl(QUrl{link});
            }
          });
  layout()->addWidget(placeholder);
}

std::filesystem::path ModusView::GetPath() const {
  return path_;
}

void ModusView::Save(WindowDefinition& definition) {
  definition.path = FullFilePathToPublic(path_);

  if (document_) {
    if (auto* sde_document = document_->sde_document()) {
      Microsoft::WRL::ComPtr<SDECore::ISDEPages> pages;
      sde_document->get_Pages(pages.ReleaseAndGetAddressOf());
      if (pages) {
        long count = 0;
        pages->get_Count(&count);
        for (long i = 0; i < count; ++i) {
          Microsoft::WRL::ComPtr<SDECore::ISDEPage50> page;
          pages->get_Item(CComVariant{i}, page.ReleaseAndGetAddressOf());
          if (page) {
            double scale = 0;
            page->get_Scale(&scale);
            definition.AddItem("Page")
                .SetInt("index", i)
                .SetInt("scale", scale * 100);
          }
        }
      }
    }
  }
}

bool ModusView::ShowContainedItem(const scada::NodeId& item_id) {
  return document_ && document_->ShowContainedItem(item_id);
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

void ModusView::ShowSetupDialog() {
  if (document_) {
    document_->sde_form().ShowOptions();
  }
}
