// scada.client.controller — named C++20 module facade over the
// client/controller headers (built for client_controller_qt).
//
// Same design and rules as scada.base (see core/base/scada_base.cppm and
// core/docs/cxx-modules.md): headers stay the source of truth, the global
// module fragment includes them, the purview re-exports names with
// `export using`. The `export import`s mirror client_controller's PUBLIC
// links (aui, client_base, client_profile, node_service, timed_data), so
// `import scada.client.controller;` provides the full transitive surface.
//
// Excluded headers (test doubles; never part of a facade):
//  - controller_delegate_mock.h, controller_factory_mock.h,
//    controller_mock.h (GMock mocks),
//  - controller_fake.h (test fake).
// No remaining header carries Windows-only content; all are included
// unconditionally.
//
// Not exported (documented; textual #include alongside the import):
//  - the REGISTER_CONTROLLER / REGISTER_CONTROLLER_FACTORY (and COMBINE*)
//    macros in controller_registry.h — macros cannot be exported. The
//    header's classes and functions (ControllerRegistrar, ControllerRegistry,
//    GetControllerRegistrar, ...) ARE exported; registration sites using the
//    macros still need a textual #include "controller/controller_registry.h"
//    alongside the import;
//  - the global-namespace operator<< for OpenContext (open_context.h) —
//    global-namespace ADL operators are never exported.
//
// node_id_set.h was checked for std::hash / std::formatter specializations:
// none (NodeIdSet is a std::set; std::hash<scada::NodeId> is owned and kept
// alive by scada.core) — no keep-alives needed in this facade.

module;

// ---- Global module fragment: headers stay the source of truth ----
#include "controller/action.h"
#include "controller/action_manager.h"
#include "controller/command_handler.h"
#include "controller/command_manager.h"
#include "controller/command_registry.h"
#include "controller/command_ui_registry.h"
#include "controller/contents_model.h"
#include "controller/controller.h"
#include "controller/controller_context.h"
#include "controller/controller_delegate.h"
#include "controller/controller_factory.h"
#include "controller/controller_factory_impl.h"
#include "controller/controller_registry.h"
#include "controller/node_id_set.h"
#include "controller/open_context.h"
#include "controller/selection_model.h"
#include "controller/time_model.h"
#include "controller/window_info.h"

export module scada.client.controller;

// Mirror client_controller's PUBLIC link transitivity.
export import scada.client.aui;
export import scada.client.base;
export import scada.client.profile;
export import scada.node_service;
export import scada.timed_data;

export {
  // action.h (incl. the unscoped CommandCategory enumerators)
  using ::Action;
  using ::CATEGORY_AGGREGATION;
  using ::CATEGORY_COUNT;
  using ::CATEGORY_CREATE;
  using ::CATEGORY_DEVICE;
  using ::CATEGORY_EDIT;
  using ::CATEGORY_EXPORT;
  using ::CATEGORY_INTERVAL;
  using ::CATEGORY_ITEM;
  using ::CATEGORY_NEW;
  using ::CATEGORY_OPEN;
  using ::CATEGORY_PERIOD;
  using ::CATEGORY_SETUP;
  using ::CATEGORY_SPECIFIC;
  using ::CATEGORY_VIEW;
  using ::CommandCategory;
  using ::Shortcut;

  // action_manager.h
  using ::ActionChangeMask;
  using ::ActionList;
  using ::ActionManager;
  using ::CanExpandCommandCategory;
  using ::GetCommandCategoryTitle;
  using ::GroupedActions;

  // command_handler.h
  using ::CommandHandler;

  // command_manager.h
  using ::CommandContextId;
  using ::CommandDescriptor;
  using ::CommandDescriptorList;
  using ::CommandManager;
  using ::CommandRegistration;
  using ::GroupedCommandDescriptors;
  using ::ResolveCommandHandler;
  using ::ToCommandDescriptor;

  // command_registry.h
  using ::BasicCommand;
  using ::BasicCommandRegistry;
  using ::Command;
  using ::CommandRegistry;
  using ::CreateUniqueCommandId;
  using ::MenuGroup;
  using ::UnaryFunction;
  using ::UnaryFunctionImpl;

  // command_ui_registry.h
  using ::CommandPlacements;
  using ::MainMenuId;
  using ::MenuContribution;
  using ::UiCommandRegistry;

  // contents_model.h
  using ::ContentsModel;

  // controller.h
  using ::Controller;

  // controller_context.h
  using ::ControllerContext;

  // controller_delegate.h
  using ::ControllerDelegate;

  // controller_factory.h
  using ::ControllerFactory;

  // controller_factory_impl.h
  using ::ControllerFactoryImpl;

  // controller_registry.h (the REGISTER_CONTROLLER* macros require the
  // textual include; see the header comment)
  using ::ControllerRegistrar;
  using ::ControllerRegistrarBase;
  using ::ControllerRegistry;
  using ::ControllerRegistryFactory;
  using ::FindControllerRegistrar;
  using ::GetControllerRegistrar;

  // node_id_set.h
  using ::MakeNodeIdSet;
  using ::NodeIdSet;

  // open_context.h (its global operator<< is deliberately not exported)
  using ::OpenContext;

  // selection_model.h
  using ::SelectionModel;
  using ::SelectionModelContext;

  // time_model.h
  using ::TimeModel;

  // window_info.h (incl. the unscoped WindowFlags enumerators;
  // g_window_infos is a legacy extern declaration with no definition in the
  // repo — exported for name parity with the header)
  using ::FindWindowInfo;
  using ::FindWindowInfoByName;
  using ::g_window_infos;
  using ::GetWindowInfo;
  using ::WIN_CAN_PRINT;
  using ::WIN_DISALLOW_NEW;
  using ::WIN_DOCKB;
  using ::WIN_INS;
  using ::WIN_REQUIRES_ADMIN;
  using ::WIN_SING;
  using ::WIN_SINGLE_ITEM;
  using ::WindowFlags;
  using ::WindowInfo;

  // The GroupCommands overload set spans action_manager.h (ActionManager)
  // and command_manager.h (CommandManager). Exported once, after all GMF
  // includes, so both overloads are captured.
  using ::GroupCommands;
}  // export
