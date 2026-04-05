#include "vidicon/display/native/qt/vidicon_display_native_view.h"

#include "aui/dialog_service.h"
#include "common_resources.h"
#include "components/write/write_service.h"
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
#include <format>

namespace {

// TODO: Combine with `ModusView::OpenPlaceholder`.
std::unique_ptr<UiView> CreateErrorPlaceholderWidget(
    QWidget* parent_widget,
    std::string_view error_message) {
  auto placeholder = std::make_unique<QLabel>(parent_widget);
  placeholder->setTextFormat(Qt::RichText);
  placeholder->setText(QString::fromWCharArray(LR"(<html><body>
    <p>Failed to load the Vidicon graphical display library.</p>
    <p>Error code: <i>%1</i>.</p>
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

std::unique_ptr<UiView> VidiconDisplayNativeView::Init(
    const WindowDefinition& definition) {
  path_ = definition.path;

  std::unique_ptr<DisplayWidget> widget;
  try {
    widget = std::make_unique<DisplayWidget>();
  } catch (const std::runtime_error& e) {
    auto error_widget = CreateErrorPlaceholderWidget(nullptr, e.what());
    widget_ = error_widget.get();
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

  widget->command_handler =
      std::bind_front(&VidiconDisplayNativeView::ExecCommand, this);

  widget_ = widget.get();
  return widget;
}

void VidiconDisplayNativeView::Save(WindowDefinition& definition) {
  definition.path = path_;
}

void VidiconDisplayNativeView::ExecCommand(const QString& command_name,
                                           const QVariantList& arguments) {
  // TODO: Introduce constants.
  if (command_name == "OpenWriteWin") {
    if (arguments.size() != 1) {
      // TODO: Log error.
      assert(false);
      return;
    }

    OpenWriteWin(arguments[0].toString(), /*manual*/ false);

  } else if (command_name == "OpenWriteManWin") {
    if (arguments.size() != 1) {
      // TODO: Log error.
      assert(false);
      return;
    }

    OpenWriteWin(arguments[0].toString(), /*manual*/ true);
  }
}

void VidiconDisplayNativeView::OpenWriteWin(const QString& data_source,
                                            bool manual) {
  auto node_id = vidicon::ToNodeId(data_source.toStdWString());

  if (node_id.is_null()) {
    dialog_service_.RunMessageBox(
        QString::fromWCharArray(L"Invalid Vidicon object address: %1.")
            .arg(data_source)
            .toStdU16String(),
        /*title*/ {}, MessageBoxMode::Error);
    return;
  }

  write_service_.ExecuteWriteDialog(dialog_service_, node_id, manual);
}