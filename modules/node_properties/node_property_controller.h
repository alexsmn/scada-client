#pragma once

#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/selection_model.h"

#include <boost/signals2/connection.hpp>
#include <memory>

#if defined(UI_QT)
#include <QPointer>
class DeviceParameterForm;
#endif

namespace scada::aui {
class Tree;
class TreeModel;
}  // namespace aui

class NodePropertyModel;

class NodePropertyController : protected ControllerContext, public Controller {
 public:
  explicit NodePropertyController(const ControllerContext& context);
  ~NodePropertyController();

  // Controller
  virtual std::unique_ptr<UiView> Init(
      const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  // The form's staged edits (backlog 1073); the legacy grid writes live and
  // never has any.
  virtual bool HasPendingChanges() const override;
  virtual Awaitable<bool> ResolvePendingChanges() override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }

 private:
  SelectionModel selection_{{timed_data_service_}};

  std::shared_ptr<NodePropertyModel> property_model_;
  std::shared_ptr<scada::aui::TreeModel> tree_model_;

  scada::aui::Tree* tree_view_ = nullptr;

#if defined(UI_QT)
  // The device-parameter form, when Init built one. Owned by the view; the
  // QPointer goes null when the view is destroyed.
  QPointer<DeviceParameterForm> form_;
#endif

  boost::signals2::scoped_connection node_deleted_connection_;
};
