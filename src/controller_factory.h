#pragma once

#include <memory>

#include "client/common_resources.h"
#include "client/controller.h"

std::unique_ptr<Controller> CreateController(unsigned command_id, const ControllerContext& context);

class ControllerRegistrarBase {
 public:
  explicit ControllerRegistrarBase(unsigned command_id);

  virtual std::unique_ptr<Controller> CreateController(const ControllerContext& context) = 0;
};

template<class ControllerClass>
class ControllerRegistrar : public ControllerRegistrarBase {
 public:
  explicit ControllerRegistrar(unsigned command_id) : ControllerRegistrarBase(command_id) {}

  virtual std::unique_ptr<Controller> CreateController(const ControllerContext& context) override {
    return std::make_unique<ControllerClass>(context);
  }
};

#define COMBINE1(X, Y) X##Y  // helper macro
#define COMBINE(X, Y) COMBINE1(X, Y)

#define REGISTER_CONTROLLER(ControllerClass, command_id) \
  static ControllerRegistrar<ControllerClass> COMBINE(g_factory_, __COUNTER__)(command_id)