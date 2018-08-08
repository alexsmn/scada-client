#pragma once

#include <memory>

#include "controller.h"

namespace ui {
class TreeModel;
}

class Tree;

class NodePropertyController : public Controller {
 public:
  explicit NodePropertyController(const ControllerContext& context);
  ~NodePropertyController();

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;

 private:
  std::unique_ptr<ui::TreeModel> model_;

  std::unique_ptr<Tree> view_;
};
