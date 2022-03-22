#pragma once

#include "controller.h"
#include "controller_context.h"
#include "common/node_state.h"
#include "node_service/node_observer.h"
#include "ui/views/controls/native_control.h"
#include "ui/views/controls/scroll_view.h"

#include <memory>

class RecordEditor;

class PropertyPageViewContents : public views::NativeControl {
 public:
  explicit PropertyPageViewContents(std::unique_ptr<RecordEditor> editor);

  // views::NativeControl
  virtual void CreateNativeControl(HWND parent_handle) override;
  virtual void NativeControlDestroyed() override;
  virtual bool DispatchNativeEvent(const base::NativeEvent& event) override;

  NodeRef node_;
  std::unique_ptr<RecordEditor> editor_;
};

class PropertyPageView : protected ControllerContext,
                         public Controller,
                         private NodeRefObserver {
 public:
  explicit PropertyPageView(const ControllerContext& context);
  ~PropertyPageView();

  // Controller
  virtual views::View* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;

 private:
  void SetNode(const NodeRef& node);

  // NodeRefObserver
  virtual void OnModelChanged(const scada::ModelChangeEvent& event) override;

  NodeRef node_;
  views::View* view_ = nullptr;
  PropertyPageViewContents* contents_ = nullptr;

  base::WeakPtrFactory<PropertyPageView> weak_ptr_factory_{this};
};
