#pragma once

#include <memory>

#include "controller.h"

namespace ui {
class TreeModel;
}

class NodePropertyModel;
class Tree;

class NodePropertyController : public Controller {
 public:
  explicit NodePropertyController(const ControllerContext& context);
  ~NodePropertyController();

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;
  virtual void Save(WindowDefinition& definition) override;

 private:
  std::unique_ptr<NodePropertyModel> property_model_;
  std::unique_ptr<ui::TreeModel> tree_model_;

  std::unique_ptr<Tree> tree_view_;
};
