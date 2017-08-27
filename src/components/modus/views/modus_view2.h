#pragma once

#include <functional>
#include <map>
#include <memory>

#include "base/files/file_path.h"
#include "libmodus/gfx/gfx.h"
#include "libmodus/render/renderer_delegate.h"
#include "components/modus/modus_view_wrapper.h"
#include "components/modus/modus_binding2.h"
#include "timed_data/timed_data_spec.h"
#include "ui/views/view.h"

namespace modus {
class Element;
class Renderer;
class Scheme;
class Shape;
}

namespace views {
class ScrollView;
}

class ModusBinding2;

class ModusView2 : public views::View,
                   public ModusViewWrapper,
                   private modus::RendererDelegate,
                   private ModusBinding2::Delegate {
 public:
  explicit ModusView2(TimedDataService& timed_data_service);
  virtual ~ModusView2();

  modus::Scheme* scheme() const { return scheme_.get(); }

  views::View* CreateParentIfNecessary();

  modus::Shape* selection() { return selection_; }
  ModusBinding2* GetBinding(modus::Shape* shape) const;

  typedef std::function<void(const rt::TimedDataSpec& spec)> SelectionSignal;
  void set_selection_signal(SelectionSignal signal) {
    selection_signal_ = std::move(signal);
  }

  typedef std::function<void(const base::FilePath& path)> NavigationSignal;
  void set_navigation_signal(NavigationSignal signal) {
    navigation_signal_ = std::move(signal);
  }

  typedef std::function<void()> DoubleClickSignal;
  void set_double_click_signal(DoubleClickSignal signal) {
    double_click_signal_ = std::move(signal);
  }

  // ModusViewWrapper
  virtual void Open(const base::FilePath& path) override;
  virtual base::FilePath GetPath() const override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;

  // views::View
  virtual gfx::Size GetPreferredSize() const override;
  virtual void OnPaint(gfx::Canvas* canvas) override;
  virtual void Layout() override;
  virtual bool IsFocusable() const override { return true; }
  virtual bool OnMousePressed(const ui::MouseEvent& event) override;
  virtual bool GetTooltipText(const gfx::Point& p,
                              base::string16* tooltip) const override;
  virtual bool OnMouseWheel(const ui::MouseWheelEvent& event) override;

  using TitleChangedHandler = std::function<void(base::StringPiece16 new_title)>;
  TitleChangedHandler title_changed_handler;

 private:
  friend class ModusBinding2;

  modus::Shape* GetShapeAt(const modus::Point& point) const;

  void CreateBindings();

  void SetSelection(modus::Shape* shape);

  void PaintSelection(gfx::Canvas& canvas, modus::Shape& shape);

  modus::Point PointToScheme(const gfx::Point& point) const;
  gfx::Rect BoundsToView(const modus::Rect& bounds) const;

  void ZoomAtPoint(const gfx::Point& point, float factor);

  // modus::RendererDelegate
  virtual void SchedulePaintShape(modus::Shape& shape) override;

  TimedDataService& timed_data_service_;

  base::FilePath path_;
  base::string16 title_;

  views::ScrollView* scroll_view_ = nullptr;

  std::unique_ptr<modus::Scheme> scheme_;
  std::unique_ptr<modus::Renderer> renderer_;

  float scale_ = 1.0f;

  std::map<modus::Shape*, std::unique_ptr<ModusBinding2>> bindings_;

  modus::Shape* selection_ = nullptr;

  SelectionSignal selection_signal_;
  NavigationSignal navigation_signal_;
  DoubleClickSignal double_click_signal_;
};