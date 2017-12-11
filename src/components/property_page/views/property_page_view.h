#pragma once

#include <memory>

#include "controller.h"
#include "ui/views/controls/native_control.h"
#include "ui/views/controls/scroll_view.h"
#include "core/configuration_types.h"
#include "common/node_ref_observer.h"

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

class PropertyPageView : public Controller,
                         private NodeRefObserver {
 public:
  explicit PropertyPageView(const ControllerContext& context);
  ~PropertyPageView();

  // Controller
  virtual views::View* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;

 private:
  void SetNode(const NodeRef& node, const NodeRef& parent);

  // NodeRefObserver
  virtual void OnModelChange(const ModelChangeEvent& event) override;

  scada::NodeId node_id_;
  views::View* view_ = nullptr;
  PropertyPageViewContents* contents_ = nullptr;

  base::WeakPtrFactory<PropertyPageView> weak_ptr_factory_{this};
};
