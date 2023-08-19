#include "components/modus/qt/modus_view.h"

#include "client_utils.h"
#include "components/modus/activex/modus.h"
#include "window_definition.h"

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

  if (auto* state = definition.FindItem("State")) {
    document_->InitFromState(RestoreBlob(state->attributes.GetString()));
  } else {
    document_->InitFromFilePath(path_);
  }

  connect(ax_widget_, SIGNAL(OnDocClick(IDispatch*, IDispatch*)), this,
          SLOT(OnDocClick(IDispatch*, IDispatch*)));
  connect(ax_widget_, SIGNAL(OnDocRightClick(IDispatch*, IDispatch*)), this,
          SLOT(OnDocRightClick(IDispatch*, IDispatch*)));
  connect(ax_widget_, SIGNAL(OnDocDblClick(IDispatch*, IDispatch*)), this,
          SLOT(OnDocDblClick(IDispatch*, IDispatch*)));
  connect(ax_widget_, SIGNAL(OnDocPopup(IDispatch*, bool&)), this,
          SLOT(OnDocPopup(IDispatch*, bool&)));

  title_callback_(document_->title());
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
  if (document_) {
    definition.AddItem("State", base::Value{SaveBlob(document_->SaveState())});
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
