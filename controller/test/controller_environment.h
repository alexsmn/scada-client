#pragma once

#include "aui/dialog_service_mock.h"
#include "aui/types.h"
#include "base/blinker_mock.h"
#include "base/test/test_executor.h"
#include "controller/command_registry.h"
#include "controller/controller_context.h"
#include "controller/controller_delegate_mock.h"
#include "controller/controller_registry.h"
#include "controller/window_info.h"
#include "core/selection_command_context.h"
#include "events/node_event_provider_mock.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_registry.h"
#include "node_service/node_service_mock.h"
#include "profile/profile.h"
#include "profile/window_definition.h"
#include "properties/property_service.h"
#include "scada/history_service_mock.h"
#include "scada/monitored_item_service_mock.h"
#include "scada/session_service_mock.h"
#include "services/create_tree.h"
#include "services/task_manager_mock.h"
#include "timed_data/timed_data_service_mock.h"

#include <gmock/gmock.h>

struct ControllerEnvironment {
  scada::services services() {
    return {
        .monitored_item_service = &monitored_item_service_,
        .history_service = &history_service_,
        .session_service = &session_service_,
    };
  }

  ControllerContext MakeControllerContext() {
    return {.executor_ = executor_,
            .controller_delegate_ = controller_delegate_,
            .task_manager_ = task_manager_,
            .session_service_ = session_service_,
            .node_event_provider_ = node_event_provider_,
            .history_service_ = history_service_,
            .monitored_item_service_ = monitored_item_service_,
            .timed_data_service_ = timed_data_service_,
            .node_service_ = node_service_,
            .file_cache_ = file_cache_,
            .profile_ = profile_,
            .dialog_service_ = dialog_service_,
            .blinker_manager_ = blinker_manager_,
            .create_tree_ = create_tree_,
            .property_service_ = property_service_};
  }

  // NOTE: Consider `ControllerTest`.
  void TestController(unsigned command_id);

  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();

  testing::NiceMock<scada::MockSessionService> session_service_;
  testing::NiceMock<scada::MockHistoryService> history_service_;
  testing::NiceMock<scada::MockMonitoredItemService> monitored_item_service_;

  ControllerRegistry controller_registry_;
  BasicCommandRegistry<SelectionCommandContext> selection_commands_;
  testing::NiceMock<MockControllerDelegate> controller_delegate_;
  testing::NiceMock<MockTaskManager> task_manager_;
  testing::NiceMock<MockNodeEventProvider> node_event_provider_;
  testing::NiceMock<MockTimedDataService> timed_data_service_;
  testing::NiceMock<MockNodeService> node_service_;
  FileRegistry file_registry_;
  FileCache file_cache_{file_registry_};
  Profile profile_;
  testing::NiceMock<MockDialogService> dialog_service_;
  testing::NiceMock<MockBlinkerManager> blinker_manager_;
  CreateTree create_tree_;
  PropertyService property_service_;
};

inline void ControllerEnvironment::TestController(unsigned command_id) {
  using namespace testing;

  auto controller_factory =
      controller_registry_.GetControllerFactory(ID_EVENT_VIEW);

  ASSERT_THAT(controller_factory, NotNull());

  auto controller = controller_factory(MakeControllerContext());

  ASSERT_THAT(controller, NotNull());

  auto view = controller->Init(/*window_def=*/{});

  ASSERT_THAT(view, NotNull());
}
