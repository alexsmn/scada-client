#include "vidicon/display/native/qt/vidicon_display_native_view.h"

#include "common_resources.h"
#include "controller/controller_delegate.h"
#include "controller/selection_model.h"
#include "profile/window_definition.h"
#include "filesystem/file_util.h"
#include "timed_data/timed_data_spec.h"
#include "vidicon/display/native/qt/display_widget.h"
#include "vidicon/teleclient/vidicon_client.h"
#include "vidicon/vidicon_node_id.h"

#include <TeleClient.h>

// VidiconDisplayNativeView

VidiconDisplayNativeView::VidiconDisplayNativeView(
    VidiconDisplayNativeViewContext&& context)
    : VidiconDisplayNativeViewContext{std::move(context)},
      selection_{{timed_data_service_}} {}

VidiconDisplayNativeView::~VidiconDisplayNativeView() = default;

UiView* VidiconDisplayNativeView::Init(const WindowDefinition& definition) {
  path_ = definition.path;

  auto widget = std::make_unique<DisplayWidget>();

  auto full_path = GetPublicFilePath(path_);
  widget->open(full_path, vidicon_client_.teleclient());

  controller_delegate_.SetTitle(full_path.stem().u16string());

  widget->setContextMenuPolicy(Qt::CustomContextMenu);
  QObject::connect(
      widget.get(), &DisplayWidget::customContextMenuRequested,
      [this, &widget = *widget](const QPoint& pos) {
        if (auto data_source = widget.dataSourceAt(pos);
            !data_source.isEmpty()) {
          if (auto node_id = vidicon::ToNodeId(data_source.toStdWString());
              !node_id.is_null()) {
            widget.setFocus();
            selection_.SelectTimedData(
                TimedDataSpec{timed_data_service_, node_id});
            controller_delegate_.ShowPopupMenu(
                IDR_ITEM_POPUP, widget.mapToGlobal(pos), /*right_click*/ true);
          }
        }
      });

  widget->shape_click_handler = [this](const QString& data_source) {
    if (auto node_id = vidicon::ToNodeId(data_source.toStdWString());
        !node_id.is_null()) {
      selection_.SelectTimedData(TimedDataSpec{timed_data_service_, node_id});
    }
  };

  widget_ = widget.get();
  return widget.release();
}

void VidiconDisplayNativeView::Save(WindowDefinition& definition) {
  definition.path = path_;
}
