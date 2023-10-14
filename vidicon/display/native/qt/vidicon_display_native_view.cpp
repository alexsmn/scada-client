#include "vidicon/display/native/qt/vidicon_display_native_view.h"

#include "controller/controller_delegate.h"
#include "controller/selection_model.h"
#include "controller/window_definition.h"
#include "filesystem/file_util.h"
#include "timed_data/timed_data_spec.h"
#include "vidicon/display/native/qt/display_widget.h"
#include "vidicon/teleclient/teleclient.h"
#include "vidicon/teleclient/vidicon_client.h"

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
          widget.setFocus();
          selection_.SelectTimedData(
              TimedDataSpec{timed_data_service_, data_source.toStdString()});
          controller_delegate_.ShowPopupMenu(0, widget.mapToGlobal(pos), true);
        }
      });

  widget->shape_click_handler = [this](const QString& data_source) {
    selection_.SelectTimedData(
        TimedDataSpec{timed_data_service_, data_source.toStdString()});
  };

  widget_ = widget.get();
  return widget.release();
}

void VidiconDisplayNativeView::Save(WindowDefinition& definition) {
  definition.path = path_;
}
