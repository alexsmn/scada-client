#pragma once

#include "common_resources.h"
#include "controller/controller.h"
#include "controller/window_info.h"

#include <memory>
#include <unordered_map>

struct ControllerContext;

// TODO: Rename to `ControllerFactory`.
using ControllerRegistryFactory =
    std::function<std::unique_ptr<Controller>(const ControllerContext&)>;

class ControllerRegistrarBase {
 public:
  ControllerRegistrarBase(const WindowInfo& window_info, bool is_static);

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
      : ControllerRegistrarBase{window_info, /*is_static=*/true} {}

  virtual std::unique_ptr<Controller> CreateController(
      const ControllerContext& context) override {
    return std::make_unique<ControllerClass>(context);
  }
};

ControllerRegistrarBase* GetControllerRegistrar(unsigned command_id);
ControllerRegistrarBase* FindControllerRegistrar(std::string_view name);

class ControllerRegistry {
 public:
  ControllerRegistry();
  ~ControllerRegistry();

  void AddControllerFactory(const WindowInfo& window_info,
                            ControllerRegistryFactory controller_factory);

  ControllerRegistryFactory GetControllerFactory(unsigned command_id) const;

 private:
  std::unordered_map<unsigned /*command_id*/, ControllerRegistrarBase*>
      registrars_;

  friend class ControllerRegistrarBase;
  friend ControllerRegistrarBase* GetControllerRegistrar(unsigned command_id);
  friend ControllerRegistrarBase* FindControllerRegistrar(
      std::string_view name);
};

#define COMBINE1(X, Y) X##Y  // helper macro
#define COMBINE(X, Y) COMBINE1(X, Y)

#define REGISTER_CONTROLLER(ControllerClass, window_info) \
  static ControllerRegistrar<ControllerClass> COMBINE(    \
      g_factory_, __COUNTER__)(window_info)

#define REGISTER_CONTROLLER_FACTORY(ControllerFactoryClass) \
  static ControllerFactoryClass COMBINE(g_factory_, __COUNTER__)
