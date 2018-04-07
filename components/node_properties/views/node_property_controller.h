#pragma once

#include <memory>

#include "controller.h"

namespace ui {
class TreeModel;
}

namespace views {
class TreeView;
}

class NodePropertyController : public Controller {
 public:
  explicit NodePropertyController(const ControllerContext& context);
  ~NodePropertyController();

  // Controller
  virtual views::View* Init(const WindowDefinition& definition) override;

 private:
  std::unique_ptr<ui::TreeModel> model_;

  std::unique_ptr<views::TreeView> view_;
};
