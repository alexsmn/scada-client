#include "vidicon/display/native/qt/vidicon_display_native_view.h"

#include "common_resources.h"
#include "controller/controller_delegate.h"
#include "controller/selection_model.h"
#include "filesystem/file_util.h"
#include "profile/window_definition.h"
#include "timed_data/timed_data_spec.h"
#include "vidicon/display/native/qt/display_widget.h"
#include "vidicon/display/native/vidicon_shape_action_menu.h"
#include "vidicon/teleclient/vidicon_client.h"
#include "vidicon/vidicon_node_id.h"

#include <QLabel>
#include <TeleClient.h>

namespace {

// TODO: Combine with `ModusView::OpenPlaceholder`.
QWidget* CreateErrorPlaceholderWidget(QWidget* parent_widget,
                                      std::string_view error_message) {
  QLabel* placeholder = new QLabel{parent_widget};
  placeholder->setTextFormat(Qt::RichText);
  placeholder->setText(QString::fromWCharArray(LR"(<html><body>
    <p>Не удалось загрузить библиотеку графических схем Видикон.</p>
    <p>Код ошибки: <i>%1</i>.</p>
    </body></html>)")
                           .arg(QString::fromLatin1(error_message.data(),
                                                    error_message.size())));
  placeholder->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
  placeholder->setWordWrap(true);
  placeholder->setTextInteractionFlags(Qt::TextBrowserInteraction);
  return placeholder;
}

}  // namespace

// VidiconDisplayNativeView

VidiconDisplayNativeView::VidiconDisplayNativeView(
    VidiconDisplayNativeViewContext&& context)
    : VidiconDisplayNativeViewContext{std::move(context)},
      selection_{{timed_data_service_}} {}

VidiconDisplayNativeView::~VidiconDisplayNativeView() = default;

UiView* VidiconDisplayNativeView::Init(const WindowDefinition& definition) {
  path_ = definition.path;

  std::unique_ptr<DisplayWidget> widget;
  try {
    widget = std::make_unique<DisplayWidget>();
  } catch (const std::runtime_error& e) {
    auto* error_widget = CreateErrorPlaceholderWidget(nullptr, e.what());
    widget_ = error_widget;
    return error_widget;
  }

  auto full_path = GetPublicFilePath(path_);
  widget->open(full_path, vidicon_client_.teleclient());

  controller_delegate_.SetTitle(full_path.stem().u16string());

  widget->setContextMenuPolicy(Qt::CustomContextMenu);

  QObject::connect(
      widget.get(), &DisplayWidget::customContextMenuRequested,
      [this, &widget = *widget](const QPoint& pos) {
        auto shape = widget.shapeAt(pos);
        if (auto actions = shape.metadata().actions; !actions.empty()) {
          VidiconShapeActionMenu action_menu{shape, actions};
          controller_delegate_.ShowPopupMenu(&action_menu.menu_model,
                                             /*resource_id*/ 0,
                                             widget.mapToGlobal(pos),
                                             /*right_click*/ true);
        }
      });

  widget->shape_click_handler = [this](const QString& data_source) {
    if (auto node_id = vidicon::ToNodeId(data_source.toStdWString());
        !node_id.is_null()) {
      selection_.SelectTimedData(TimedDataSpec{timed_data_service_, node_id});
    }
  };

  widget->command_handler = [this](const QString& command_name,
                                   const QVariantList& arguments) {
    if (command_name == "OpenWriteWin") {
      if (arguments.size() != 1) {
        // TODO: Log error.
        assert(false);
        return;
      }

      QString data_source = arguments[0].toString();
      // controller_delegate_.ExecCommand("Write", data_source);
    }
  };

  widget_ = widget.get();
  return widget.release();
}

void VidiconDisplayNativeView::Save(WindowDefinition& definition) {
  definition.path = path_;
}
