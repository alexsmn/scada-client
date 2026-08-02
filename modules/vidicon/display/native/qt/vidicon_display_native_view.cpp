#include "vidicon/display/native/qt/vidicon_display_native_view.h"

#include "aui/dialog_service.h"
#include "controller/controller_delegate.h"
#include "controller/selection_model.h"
#include "display_frame/qt/display_frame.h"
#include "filesystem/file_util.h"
#include "modules/write/write_service.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"
#include "timed_data/timed_data_spec.h"
#include "vds_runtime/qt/vds_runtime_widget.h"

#include <exception>

// VidiconDisplayNativeView

VidiconDisplayNativeView::VidiconDisplayNativeView(
    VidiconDisplayNativeViewContext&& context)
    : VidiconDisplayNativeViewContext{std::move(context)},
      selection_{{timed_data_service_}} {}

VidiconDisplayNativeView::~VidiconDisplayNativeView() = default;

std::unique_ptr<UiView> VidiconDisplayNativeView::Init(
    const WindowDefinition& definition) {
  path_ = definition.path;

  auto widget = std::make_unique<VdsRuntimeWidget>();

  auto full_path = GetPublicFilePath(path_);
  widget->Open(full_path, TC_VDS_RUNTIME_DOCUMENT_KIND_VDS);

  const QString title =
      widget->title().isEmpty()
          ? QString::fromStdU16String(full_path.stem().u16string())
          : widget->title();
  controller_delegate_.SetTitle(title.toStdU16String());

  widget->set_selection_callback([this](const QString& data_source) {
    try {
      auto node_id = scada::NodeId::FromString(data_source.toStdString());
      if (node_id.is_null())
        return;
      selection_.SelectTimedData(TimedDataSpec{timed_data_service_, node_id});
      // Mirror the selection into the frame's Measurements strip (no-op under
      // the legacy theme, where there is no frame).
      if (frame_)
        frame_->ShowMeasurement(node_id);
    } catch (const std::exception&) {
      selection_.Clear();
    }
  });

  widget_ = widget.get();

  // Reshell chrome (opt-in): wrap the renderer in the display frame — Live
  // indicator, hotspot breadcrumb, zoom / fit / export, and the bay strips —
  // when the UX theme is active. Under the legacy theme WrapDisplayInFrame
  // returns the bare renderer unchanged. The frame reparents (owns) the
  // renderer, so release the unique_ptr only when ownership actually moved into
  // a new frame.
  QWidget* framed = WrapDisplayInFrame(
      widget.get(), title,
      DisplayFrameContext{.timed_data_service = &timed_data_service_,
                          .node_event_provider = &node_event_provider_,
                          .node_service = &node_service_});
  if (framed == widget_)
    return widget;

  frame_ = static_cast<DisplayFrame*>(framed);
  widget.release();
  return std::unique_ptr<UiView>{framed};
}

void VidiconDisplayNativeView::Save(WindowDefinition& definition) {
  definition.path = path_;
}

void VidiconDisplayNativeView::ExecCommand(const QString& command_name,
                                           const QVariantList& arguments) {
  // TODO: Introduce constants.
  if (command_name == "OpenWriteWin") {
    if (arguments.size() != 1) {
      // Display command arguments are external data. TODO: Log error.
      return;
    }

    OpenWriteWin(arguments[0].toString(), /*manual*/ false);

  } else if (command_name == "OpenWriteManWin") {
    if (arguments.size() != 1) {
      // Display command arguments are external data. TODO: Log error.
      return;
    }

    OpenWriteWin(arguments[0].toString(), /*manual*/ true);
  }
}

void VidiconDisplayNativeView::OpenWriteWin(const QString& data_source,
                                            bool manual) {
  scada::NodeId node_id;
  try {
    node_id = scada::NodeId::FromString(data_source.toStdString());
  } catch (const std::exception&) {
  }

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
