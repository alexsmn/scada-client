#pragma once

#include <memory>

#include "common_resources.h"
#include "controller.h"
#include "window_info.h"

class ControllerDelegate;
class DialogService;

using ControllerFactory =
    std::function<std::unique_ptr<Controller>(unsigned command_id,
                                              ControllerDelegate& delegate,
                                              DialogService& dialog_service)>;

class ControllerRegistrarBase {
 public:
  explicit ControllerRegistrarBase(const WindowInfo& window_info);

  const WindowInfo& window_info() const { return window_info_; }

  virtual std::unique_ptr<Controller> CreateController(
      const ControllerContext& context) = 0;

 private:
  const WindowInfo& window_info_;
};

template <class ControllerClass>
class ControllerRegistrar final : public ControllerRegistrarBase {
 public:
  explicit ControllerRegistrar(const WindowInfo& window_info)
      : ControllerRegistrarBase{window_info} {}

  virtual std::unique_ptr<Controller> CreateController(
      const ControllerContext& context) override {
    return std::make_unique<ControllerClass>(context);
  }
};

ControllerRegistrarBase* GetControllerRegistrar(unsigned command_id);
ControllerRegistrarBase* FindControllerRegistrar(std::string_view name);

#define COMBINE1(X, Y) X##Y  // helper macro
#define COMBINE(X, Y) COMBINE1(X, Y)

#define REGISTER_CONTROLLER(ControllerClass, window_info) \
  static ControllerRegistrar<ControllerClass> COMBINE(    \
      g_factory_, __COUNTER__)(window_info)
