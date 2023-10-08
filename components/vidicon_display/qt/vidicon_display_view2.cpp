#include "components/vidicon_display/qt/vidicon_display_view2.h"

#include "components/vidicon_display/qt/gdi_widget.h"
#include "components/vidicon_display/qt/gdi_widget2.h"
#include "components/vidicon_display/qt/vidicon_display_lib.h"
#include "controller/controller_delegate.h"
#include "controller/file_util.h"
#include "controller/selection_model.h"
#include "controller/window_definition.h"
#include "services/vidicon/teleclient.h"
#include "services/vidicon/vidicon_client.h"
#include "timed_data/timed_data_spec.h"

namespace {

inline POINT MakePOINT(const QPoint& p) {
  return {p.x(), p.y()};
}

// Lazy initialization.
inline const vidicon::display_library& GetDisplayLib() {
  static const vidicon::display_library lib;
  return lib;
}

}  // namespace

// DisplayWidget

class DisplayWidget : public GdiWidget2 {
 public:
  explicit DisplayWidget(QWidget* parent = nullptr) : GdiWidget2{parent} {
    display_.set_invalidate_handler([this](const vidicon::display_rect& rect) {
      update(ToQRect(display_.viewport_rect(display_viewport(), rect)));
    });
  }

  void open(const std::filesystem::path& path, IClient& teleclient) {
    display_.open(path, teleclient);
  }

  QString dataSourceAt(const QPoint& p) const {
    auto shape = display_.shape_at(display_viewport(), MakePOINT(p));
    return QString::fromStdWString(shape.metadata().data_source);
  }

  virtual void mousePressEvent(QMouseEvent* event) override {
    setFocus();

    if (shape_click_handler) {
      if (auto data_source = dataSourceAt(event->pos());
          !data_source.isEmpty()) {
        shape_click_handler(data_source);
      }
    }

    GdiWidget2::mousePressEvent(event);
  }

  using ShapeClickHandler = std::function<void(const QString& data_source)>;
  ShapeClickHandler shape_click_handler;

 protected:
  vidicon::display_viewport display_viewport() const {
    return {.rect = {.right = width(), .bottom = height()}};
  }

  virtual void paint(HDC dc, const RECT& rect) override {
    display_.draw(dc, vidicon::display_viewport{.rect = rect});
  }

  virtual QString tooltipAt(const QPoint& p) const override {
    return dataSourceAt(p);
  }

 private:
  vidicon::display display_{GetDisplayLib()};
};

// VidiconDisplayView2

VidiconDisplayView2::VidiconDisplayView2(VidiconDisplayView2Context&& context)
    : VidiconDisplayView2Context{std::move(context)},
      selection_{{timed_data_service_}} {}

VidiconDisplayView2::~VidiconDisplayView2() = default;

UiView* VidiconDisplayView2::Init(const WindowDefinition& definition) {
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

void VidiconDisplayView2::Save(WindowDefinition& definition) {
  definition.path = path_;
}
