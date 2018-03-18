#pragma once

#include <memory>

#include "controller.h"

namespace views {
class PropertyView;
}

namespace ui {
class PropertyListModel;
}

class NodePropertyController : public Controller {
 public:
  explicit NodePropertyController(const ControllerContext& context);
  virtual ~NodePropertyController();

  // Controller
  virtual views::View* Init(const WindowDefinition& definition) override;

 private:
  std::unique_ptr<ui::PropertyListModel> model_;

  std::unique_ptr<views::PropertyView> view_;
};
