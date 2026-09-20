#include "modus/qt/modus_view2.h"

#include "base/win/scoped_gdi_object.h"
#include "base/win/scoped_hdc.h"
#include "filesystem/file_util.h"
#include "libmodus/gfx/canvas.h"
#include "libmodus/render/renderer.h"
#include "libmodus/render/shape.h"
#include "libmodus/scheme/element.h"
#include "libmodus/scheme/property_def.h"
#include "libmodus/scheme/scheme.h"
#include "libmodus/scheme/serialization.h"
#include "libmodus/scheme/value.h"
#include "modus/libmodus/modus_binding2.h"
#include "modus/libmodus/modus_module2.h"
#include "profile/window_definition.h"

#include <QImage>
#include <QMouseEvent>
#include <QPaintEngine>
#include <QPainter>
#include <QRegion>

namespace {
const int kSelectionInset = 3;
const float kHitTolerance = 6.0f;
}  // namespace

ModusView2::ModusView2(TimedDataService& timed_data_service)
    : timed_data_service_{timed_data_service} {}

ModusView2::~ModusView2() {}

ModusBinding2* ModusView2::GetBinding(scada::modus::Shape* shape) const {
  if (!shape)
    return nullptr;
  auto i = bindings_.find(shape);
  return i == bindings_.end() ? nullptr : i->second.get();
}

void ModusView2::Open(const WindowDefinition& definition) {
  path_ = GetPublicFilePath(definition.path);

  auto& master_library = ModusModule2::GetInstance()->master_library();
  scheme_ = std::make_unique<scada::modus::Scheme>();
  scheme_->set_master_library(&master_library);
  // TODO: Check result.
  scada::modus::LoadScheme(*scheme_, path_);

  if (scheme_) {
    title_ = scheme_->GetValue(scada::modus::kAttrSchemeTitle).as_string();
    renderer_.reset(new scada::modus::Renderer(*scheme_, *this));
    CreateBindings();
  }

  updateGeometry();
}

std::filesystem::path ModusView2::GetPath() const {
  return path_;
}

void ModusView2::Save(WindowDefinition& definition) {}

bool ModusView2::ShowContainedItem(const scada::NodeId& item_id) {
  // TODO:
  return false;
}

QSize ModusView2::sizeHint() const {
  if (!scheme_)
    return QSize();

  return QSize(static_cast<int>(renderer_->size().width() * scale_),
               static_cast<int>(renderer_->size().height() * scale_));
}

void ModusView2::paintEvent(QPaintEvent* e) {
  if (!renderer_)
    return;

  QPainter painter(this);

  {
    scada::base::win::ScopedCreateDC dc(::CreateCompatibleDC(GetDC(0)));
    scada::base::win::ScopedBitmap bitmap(
        ::CreateCompatibleBitmap(GetDC(0), width(), height()));
    ::SelectObject(dc.Get(), bitmap.get());

    Gdiplus::Graphics graphics(dc.Get());
    // graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

    scada::base::win::ScopedRegion clip_region(painter.clipRegion().toHRGN());
    graphics.SetClip(clip_region.get());

    graphics.Clear(static_cast<Gdiplus::ARGB>(Gdiplus::Color::White));

    graphics.ScaleTransform(scale_, scale_);

    for (auto& p : bindings_)
      p.second->Paint(graphics, true);

    scada::modus::Canvas c(graphics);
    renderer_->Paint(c);

    for (auto& p : bindings_)
      p.second->Paint(graphics, false);

    auto pixmap = QPixmap::fromImage(QImage::fromHBITMAP(bitmap.get()));
    painter.drawPixmap(0, 0, pixmap);
  }

  if (selection_)
    PaintSelection(painter, *selection_);
}

/*void ModusView2::Layout() {
  SizeToPreferredSize();
}*/

void ModusView2::mousePressEvent(QMouseEvent* e) {
  setFocus(Qt::MouseFocusReason);

  auto point = PointToScheme(e->pos());
  auto* shape = GetShapeAt(point);
  if (!shape)
    return;

  SetSelection(shape);
  e->accept();
}

void ModusView2::mouseDoubleClickEvent(QMouseEvent* e) {
  setFocus(Qt::MouseFocusReason);

  auto point = PointToScheme(e->pos());
  auto* shape = GetShapeAt(point);
  if (!shape)
    return;

  auto link = shape->element().GetValue("Links[0]");
  if (!link.empty() && navigation_signal_) {
    navigation_signal_(std::filesystem::path(
        scada::modus::GetLinkFilePath(link.as_string_view())));
  }

  if (double_click_signal_)
    double_click_signal_();
}

/*bool ModusView2::GetTooltipText(const gfx::Point& p,
                                std::wstring* tooltip) const {
  auto point = PointToScheme(p);
  auto shape = GetShapeAt(point);
  auto binding = GetBinding(shape);
  if (!binding)
    return false;

  *tooltip = GetTimedDataTooltipText(binding->data_point());
  return true;
}

bool ModusView2::OnMouseWheel(const ui::MouseWheelEvent& event) {
  float factor = event.offset() > 0 ? 1.1f : 0.9f;
  ZoomAtPoint(event.location(), factor);
  return true;
}*/

void ModusView2::CreateBindings() {
  for (auto& shape : renderer_->shapes()) {
    auto binding = shape->element().GetValue("Tech.keyLink");
    if (!binding.empty()) {
      bindings_[shape.get()].reset(new ModusBinding2(
          *this, *shape, binding.as_string(), timed_data_service_));
    }
  }
}

void ModusView2::SetSelection(scada::modus::Shape* shape) {
  if (shape && !GetBinding(shape))
    shape = nullptr;

  if (selection_ == shape)
    return;

  if (selection_)
    SchedulePaintShape(*selection_);

  selection_ = shape;

  if (selection_)
    SchedulePaintShape(*selection_);

  if (selection_signal_) {
    auto binding = GetBinding(selection_);
    TimedDataSpec spec = binding ? binding->data_point() : TimedDataSpec();
    selection_signal_(spec);
  }
}

void ModusView2::SchedulePaintShape(scada::modus::Shape& shape) {
  auto inflate = std::max(kSelectionInset, kModusBindingInflate) + 1;

  {
    auto rect = BoundsToView(shape.bounds());
    rect.adjust(-inflate, -inflate, inflate, inflate);
    update(rect);
  }

  {
    auto rect = BoundsToView(shape.GetTextBounds());
    if (!rect.isEmpty()) {
      rect.adjust(-inflate, -inflate, inflate, inflate);
      update(rect);
    }
  }
}

void ModusView2::PaintSelection(QPainter& painter, scada::modus::Shape& shape) {
  {
    auto rect = BoundsToView(shape.bounds());
    if (!rect.isEmpty()) {
      rect.adjust(-kSelectionInset, -kSelectionInset, kSelectionInset,
                  kSelectionInset);
      painter.drawRect(rect);
    }
  }

  {
    auto rect = BoundsToView(shape.GetTextBounds());
    if (!rect.isEmpty()) {
      rect.adjust(-kSelectionInset, -kSelectionInset, kSelectionInset,
                  kSelectionInset);
      painter.drawRect(rect);
    }
  }
}

scada::modus::Point ModusView2::PointToScheme(const QPoint& point) const {
  return scada::modus::Point(point.x() / scale_, point.y() / scale_);
}

QRect ModusView2::BoundsToView(const scada::modus::Rect& bounds) const {
  return QRect(floor(bounds.x() * scale_), floor(bounds.y() * scale_),
               ceil(bounds.width() * scale_), ceil(bounds.height() * scale_));
}

void ModusView2::ZoomAtPoint(const QPoint& point, float factor) {
  /*  auto visible_bounds = GetVisibleBounds();

    scale_ *= factor;

    update();
    updateGeometry();

    QRect new_visible_bounds(
        visible_bounds.x() * factor,
        visible_bounds.y() * factor,
        visible_bounds.width() * factor,
        visible_bounds.height() * factor);
    ScrollRectToVisible(new_visible_bounds);*/
}

scada::modus::Shape* ModusView2::GetShapeAt(
    const scada::modus::Point& point) const {
  return renderer_->GetShapeAt(point, kHitTolerance);
}
