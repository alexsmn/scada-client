#include "components/modus/views/modus_view2.h"

#include "base/strings/string_split.h"
#include "base/win/scoped_gdi_object.h"
#include "client_utils.h"
#include "components/modus/modus_binding2.h"
#include "components/modus/modus_module2.h"
#include "libmodus/gfx/canvas.h"
#include "libmodus/render/renderer.h"
#include "libmodus/render/shape.h"
#include "libmodus/scheme/element.h"
#include "libmodus/scheme/property_def.h"
#include "libmodus/scheme/scheme.h"
#include "libmodus/scheme/serialization.h"
#include "libmodus/scheme/value.h"
#include "ui/gfx/canvas.h"
#include "ui/native_theme/native_theme.h"
#include "ui/views/background.h"
#include "ui/views/controls/scroll_view.h"

namespace {
const int kSelectionInset = 3;
const float kHitTolerance = 6.0f;
}  // namespace

ModusView2::ModusView2(ModusView2Context&& context)
    : ModusView2Context{std::move(context)} {}

ModusView2::~ModusView2() {}

views::View* ModusView2::CreateParentIfNecessary() {
  scroll_view_ = new views::ScrollView;
  scroll_view_->SetContentsView(this);
  scroll_view_->set_background(new views::ThemedBackground(
      *scroll_view_, ui::NativeTheme::kColorWindow));
  return scroll_view_;
}

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
    if (title_changed_handler)
      title_changed_handler(title_);

    renderer_.reset(new modus::Renderer(*scheme_, *this));
    CreateBindings();
  }

  PreferredSizeChanged();
}

base::FilePath ModusView2::GetPath() const {
  return path_;
}

bool ModusView2::ShowContainedItem(const scada::NodeId& item_id) {
  // TODO:
  return false;
}

gfx::Size ModusView2::GetPreferredSize() const {
  if (!scheme_)
    return gfx::Size();

  return gfx::Size(static_cast<int>(renderer_->size().width() * scale_),
                   static_cast<int>(renderer_->size().height() * scale_));
}

void ModusView2::OnPaint(gfx::Canvas* canvas) {
  if (!renderer_)
    return;

  {
    Gdiplus::Graphics graphics(canvas->native_canvas());
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);

    base::win::ScopedRegion clip_region(::CreateRectRgn(0, 0, 0, 0));
    if (::GetClipRgn(canvas->native_canvas(), clip_region.get()))
      graphics.SetClip(clip_region.get());

    Gdiplus::Color page_color;
    page_color.SetFromCOLORREF(renderer_->page_color());
    graphics.Clear(page_color);
    graphics.ScaleTransform(scale_, scale_);

    for (auto& p : bindings_)
      p.second->Paint(graphics, true);

    modus::Canvas c(graphics);
    renderer_->Paint(c);

    for (auto& p : bindings_)
      p.second->Paint(graphics, false);
  }

  if (selection_)
    PaintSelection(*canvas, *selection_);
}

void ModusView2::Layout() {
  SizeToPreferredSize();
}

bool ModusView2::OnMousePressed(const ui::MouseEvent& event) {
  auto point = PointToScheme(event.location());
  auto* shape = GetShapeAt(point);
  if (!shape)
    return false;

  SetSelection(shape);

  if (event.IsDoubleClick()) {
    auto link = shape->element().GetValue("Links[0]");
    if (!link.empty() && navigation_signal_)
      navigation_signal_(
          base::FilePath(modus::GetLinkFilePath(link.as_string_piece())));

    if (double_click_signal_)
      double_click_signal_();
  }

  return false;
}

bool ModusView2::GetTooltipText(const gfx::Point& p,
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
}

void ModusView2::CreateBindings() {
  for (auto& shape : renderer_->shapes()) {
    auto composite_binding_string =
        shape->element().GetValue("Tech.keyLink").as_string();
    auto binding_string_list =
        base::SplitString(composite_binding_string, L";",
                          base::WhitespaceHandling::KEEP_WHITESPACE,
                          base::SplitResult::SPLIT_WANT_NONEMPTY);
    for (auto& binding_string : binding_string_list) {
      ModusBinding2::Delegate& delegate = *this;
      auto binding = std::make_unique<ModusBinding2>(
          delegate, *shape, binding_string, timed_data_service_);
      bindings_.emplace(shape.get(), std::move(binding));
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
    rt::TimedDataSpec spec =
        binding ? binding->data_point() : rt::TimedDataSpec();
    selection_signal_(spec);
  }
}

void ModusView2::SchedulePaintShape(modus::Shape& shape) {
  auto inflate = std::max(kSelectionInset, kModusBindingInflate);

  {
    gfx::Rect rect = BoundsToView(shape.bounds());
    rect.Inset(-inflate, -inflate);
    SchedulePaint(rect);
  }

  {
    gfx::Rect rect = BoundsToView(shape.GetTextBounds());
    if (!rect.IsEmpty()) {
      rect.Inset(-inflate, -inflate);
      SchedulePaint(rect);
    }
  }
}

void ModusView2::PaintSelection(gfx::Canvas& canvas, modus::Shape& shape) {
  {
    auto rect = BoundsToView(shape.bounds());
    if (!rect.IsEmpty()) {
      rect.Inset(-kSelectionInset, -kSelectionInset);
      canvas.DrawRect(rect, SK_ColorBLACK);
    }
  }

  {
    auto rect = BoundsToView(shape.GetTextBounds());
    if (!rect.IsEmpty()) {
      rect.Inset(-kSelectionInset, -kSelectionInset);
      canvas.DrawRect(rect, SK_ColorBLACK);
    }
  }
}

modus::Point ModusView2::PointToScheme(const gfx::Point& point) const {
  return modus::Point(point.x() / scale_, point.y() / scale_);
}

gfx::Rect ModusView2::BoundsToView(const modus::Rect& bounds) const {
  return gfx::Rect(floor(bounds.x() * scale_), floor(bounds.y() * scale_),
                   ceil(bounds.width() * scale_),
                   ceil(bounds.height() * scale_));
}

void ModusView2::ZoomAtPoint(const gfx::Point& point, float factor) {
  auto visible_bounds = GetVisibleBounds();

  scale_ *= factor;

  SchedulePaint();
  PreferredSizeChanged();

  gfx::Rect new_visible_bounds(
      visible_bounds.x() * factor, visible_bounds.y() * factor,
      visible_bounds.width() * factor, visible_bounds.height() * factor);
  ScrollRectToVisible(new_visible_bounds);
}

modus::Shape* ModusView2::GetShapeAt(const modus::Point& point) const {
  return renderer_->GetShapeAt(point, kHitTolerance);
}
