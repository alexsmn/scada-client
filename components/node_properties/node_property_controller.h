#pragma once

#include "controller.h"
#include "controller_context.h"
#include "selection_model.h"

#include <boost/signals2/connection.hpp>
#include <memory>

namespace ui {
class TreeModel;
}

class NodePropertyModel;
class Tree;

class NodePropertyController : protected ControllerContext, public Controller {
 public:
  explicit NodePropertyController(const ControllerContext& context);
  ~NodePropertyController();

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;
  virtual SelectionModel* GetSelectionModel() override { return &selection_; }

 private:
  SelectionModel selection_{{timed_data_service_}};

  std::shared_ptr<NodePropertyModel> property_model_;
  std::shared_ptr<ui::TreeModel> tree_model_;

  Tree* tree_view_;

  boost::signals2::scoped_connection node_deleted_connection_;
};
