#include "components/modus/qt/modus_view2.h"

#include <QtWinExtras/qwinfunctions.h>
#include <qevent.h>
#include <qpaintengine.h>
#include <qpainter.h>

#include "base/win/scoped_gdi_object.h"
#include "base/win/scoped_hdc.h"
#include "client_utils.h"
#include "components/modus/libmodus/modus_binding2.h"
#include "components/modus/libmodus/modus_module2.h"
#include "libmodus/gfx/canvas.h"
#include "libmodus/render/renderer.h"
#include "libmodus/render/shape.h"
#include "libmodus/scheme/element.h"
#include "libmodus/scheme/property_def.h"
#include "libmodus/scheme/scheme.h"
#include "libmodus/scheme/serialization.h"
#include "libmodus/scheme/value.h"

namespace {
const int kSelectionInset = 3;
const float kHitTolerance = 6.0f;
}  // namespace

ModusView2::ModusView2(TimedDataService& timed_data_service)
    : timed_data_service_{timed_data_service} {}

ModusView2::~ModusView2() {}

ModusBinding2* ModusView2::GetBinding(modus::Shape* shape) const {
  if (!shape)
    return nullptr;
  auto i = bindings_.find(shape);
  return i == bindings_.end() ? nullptr : i->second.get();
}

void ModusView2::Open(const base::FilePath& path) {
  path_ = path;

  auto& master_library = ModusModule2::GetInstance()->master_library();
  scheme_ = modus::LoadScheme(path.value(), master_library);

  if (scheme_) {
    title_ = scheme_->GetValue(modus::kAttrSchemeTitle).as_string();
    renderer_.reset(new modus::Renderer(*scheme_, *this));
    CreateBindings();
  }

  updateGeometry();
}

base::FilePath ModusView2::GetPath() const {
  return path_;
}

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
    base::win::ScopedCreateDC dc(::CreateCompatibleDC(GetDC(0)));
    base::win::ScopedBitmap bitmap(
        ::CreateCompatibleBitmap(GetDC(0), width(), height()));
    ::SelectObject(dc.Get(), bitmap.get());

    Gdiplus::Graphics graphics(dc.Get());
    // graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

    base::win::ScopedRegion clip_region(QtWin::toHRGN(painter.clipRegion()));
    graphics.SetClip(clip_region.get());

    graphics.Clear(Gdiplus::Color::White);

    graphics.ScaleTransform(scale_, scale_);

    for (auto& p : bindings_)
      p.second->Paint(graphics, true);

    modus::Canvas c(graphics);
    renderer_->Paint(c);

    for (auto& p : bindings_)
      p.second->Paint(graphics, false);

    auto pixmap = QtWin::fromHBITMAP(bitmap.get());
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
    navigation_signal_(
        base::FilePath(modus::GetLinkFilePath(link.as_string_view())));
  }

  if (double_click_signal_)
    double_click_signal_();
}

/*bool ModusView2::GetTooltipText(const gfx::Point& p,
                                base::string16* tooltip) const {
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

void ModusView2::SetSelection(modus::Shape* shape) {
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

void ModusView2::SchedulePaintShape(modus::Shape& shape) {
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

void ModusView2::PaintSelection(QPainter& painter, modus::Shape& shape) {
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

modus::Point ModusView2::PointToScheme(const QPoint& point) const {
  return modus::Point(point.x() / scale_, point.y() / scale_);
}

QRect ModusView2::BoundsToView(const modus::Rect& bounds) const {
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

modus::Shape* ModusView2::GetShapeAt(const modus::Point& point) const {
  return renderer_->GetShapeAt(point, kHitTolerance);
}

htsde2::IHTSDEForm2* ModusView2::GetSdeForm() {
  return nullptr;
}
