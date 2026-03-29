#pragma once

#include "modus/libmodus/modus_binding2.h"
#include "modus/modus_view_wrapper.h"
#include "libmodus/gfx/gfx.h"
#include "libmodus/render/renderer_delegate.h"
#include "timed_data/timed_data_spec.h"

#include <functional>
#include <map>
#include <memory>
#include <qwidget.h>

namespace modus {
class Element;
class Renderer;
class Scheme;
class Shape;
}  // namespace modus

class ModusBinding2;
class TimedDataService;

// Uses libmodus.
class ModusView2 : public QWidget,
                   public ModusViewWrapper,
                   private modus::RendererDelegate,
                   private ModusBinding2::Delegate {
 public:
  explicit ModusView2(TimedDataService& timed_data_service);
  virtual ~ModusView2();

  modus::Scheme* scheme() const { return scheme_.get(); }

  modus::Shape* selection() { return selection_; }
  ModusBinding2* GetBinding(modus::Shape* shape) const;

  typedef std::function<void(const TimedDataSpec& spec)> SelectionSignal;
  void set_selection_signal(SelectionSignal signal) {
    selection_signal_ = std::move(signal);
  }

  typedef std::function<void(const std::filesystem::path& path)>
      NavigationSignal;
  void set_navigation_signal(NavigationSignal signal) {
    navigation_signal_ = std::move(signal);
  }

  typedef std::function<void()> DoubleClickSignal;
  void set_double_click_signal(DoubleClickSignal signal) {
    double_click_signal_ = std::move(signal);
  }

  // ModusViewWrapper
  virtual void Open(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual std::filesystem::path GetPath() const override;
  virtual bool ShowContainedItem(const scada::NodeId& item_id) override;

  // QWidget
  virtual QSize sizeHint() const override;
  virtual void paintEvent(QPaintEvent* e) override;
  virtual void mousePressEvent(QMouseEvent* e) override;
  virtual void mouseDoubleClickEvent(QMouseEvent* e) override;
  /*virtual void Layout() override;
  virtual bool IsFocusable() const override { return true; }
  virtual bool GetTooltipText(const gfx::Point& p,
                              std::wstring* tooltip) const override;
  virtual bool OnMouseWheel(const ui::MouseWheelEvent& event) override;*/

 private:
  friend class ModusBinding2;

  void Paint();

  modus::Shape* GetShapeAt(const modus::Point& point) const;

  void CreateBindings();

  void SetSelection(modus::Shape* shape);

  void PaintSelection(QPainter& painter, modus::Shape& shape);

  modus::Point PointToScheme(const QPoint& point) const;
  QRect BoundsToView(const modus::Rect& bounds) const;

  void ZoomAtPoint(const QPoint& point, float factor);

  // modus::RendererDelegate
  virtual void SchedulePaintShape(modus::Shape& shape) override;

  TimedDataService& timed_data_service_;

  std::filesystem::path path_;
  std::wstring title_;

  std::unique_ptr<modus::Scheme> scheme_;
  std::unique_ptr<modus::Renderer> renderer_;

  float scale_ = 1.0f;

  std::map<modus::Shape*, std::unique_ptr<ModusBinding2>> bindings_;

  modus::Shape* selection_ = nullptr;

  SelectionSignal selection_signal_;
  NavigationSignal navigation_signal_;
  DoubleClickSignal double_click_signal_;
};
