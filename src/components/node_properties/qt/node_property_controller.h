#pragma once

#include <memory>

#include "controller.h"

class NodePropertyModel;
class QTreeView;
class TreeModelAdapter;

class NodePropertyController : public Controller {
 public:
  explicit NodePropertyController(const ControllerContext& context);
  virtual ~NodePropertyController();

  // Controller
  virtual UiView* Init(const WindowDefinition& definition) override;

 private:
  std::unique_ptr<NodePropertyModel> model_;
  std::unique_ptr<TreeModelAdapter> model_adapter_;
  QTreeView* view_;
};
