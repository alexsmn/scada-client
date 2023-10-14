#pragma once

#include "vidicon/display/native/qt/gdi_widget2.h"
#include "vidicon/display/native/vidicon_display_lib.h"

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
    auto shape = display_.shape_at(display_viewport(), ToPOINT(p));
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
  // Lazy initialization.
  inline static const vidicon::display_library& GetDisplayLib() {
    static const vidicon::display_library lib;
    return lib;
  }

  vidicon::display display_{GetDisplayLib()};
};
