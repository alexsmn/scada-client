#include "vidicon/display/native/qt/vidicon_display_native_view.h"

#include "aui/dialog_service.h"
#include "controller/controller_delegate.h"
#include "controller/selection_model.h"
#include "filesystem/file_util.h"
#include "modules/write/write_service.h"
#include "profile/window_definition.h"
#include "resources/common_resources.h"
#include "timed_data/timed_data_spec.h"
#include "vds_runtime/qt/vds_runtime_widget.h"
#include "vidicon/teleclient/vidicon_client.h"
#include "vidicon/vidicon_node_id.h"

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

  controller_delegate_.SetTitle(widget->title().isEmpty()
                                    ? full_path.stem().u16string()
                                    : widget->title().toStdU16String());

  widget->set_selection_callback([this](const QString& data_source) {
    if (auto node_id = vidicon::ToNodeId(data_source.toStdWString());
        !node_id.is_null()) {
      selection_.SelectTimedData(TimedDataSpec{timed_data_service_, node_id});
    }
  });

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
