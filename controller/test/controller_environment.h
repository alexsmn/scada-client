#pragma once

#include "base/blinker_mock.h"
#include "base/test/test_executor.h"
#include "common/aliases_mock.h"
#include "components/favourites/favourites.h"
#include "components/portfolio/portfolio_manager.h"
#include "controller/controller_delegate_mock.h"
#include "events/node_event_provider_mock.h"
#include "node_service/node_service_mock.h"
#include "scada/history_service_mock.h"
#include "scada/monitored_item_service_mock.h"
#include "scada/session_service_mock.h"
#include "services/create_tree.h"
#include "services/dialog_service_mock.h"
#include "filesystem/file_cache.h"
#include "filesystem/file_registry.h"
#include "services/local_events.h"
#include "profile/profile.h"
#include "services/properties/property_service.h"
#include "services/task_manager_mock.h"
#include "timed_data/timed_data_service_mock.h"
#include "profile/window_definition.h"
#include "controller/window_info.h"

#if !defined(UI_WT)
#include "vidicon/teleclient/vidicon_client.h"
#endif

#include <gmock/gmock.h>

struct ControllerEnvironment {
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  testing::NiceMock<MockControllerDelegate> controller_delegate_;
  testing::NiceMock<MockAliasResolver> alias_resolver_;
  testing::NiceMock<MockTaskManager> task_manager_;
  testing::NiceMock<scada::MockSessionService> session_service_;
  testing::NiceMock<MockNodeEventProvider> node_event_provider_;
  testing::NiceMock<scada::MockHistoryService> history_service_;
  testing::NiceMock<scada::MockMonitoredItemService> monitored_item_service_;
  testing::NiceMock<MockTimedDataService> timed_data_service_;
  testing::NiceMock<MockNodeService> node_service_;
  PortfolioManager portfolio_manager_{{.node_service_ = node_service_}};
  LocalEvents local_events_;
  Favourites favourites_;
  FileRegistry file_registry_;
  FileCache file_cache_{file_registry_};
  Profile profile_;
  testing::NiceMock<MockDialogService> dialog_service_;
  testing::NiceMock<MockBlinkerManager> blinker_manager_;
  CreateTree create_tree_;
  PropertyService property_service_;

#if !defined(UI_WT)
  vidicon::VidiconClient vidicon_client_{
      {.executor_ = executor_, .timed_data_service_ = timed_data_service_}};
#endif

  ControllerContext MakeControllerContext() {
    return {
      .executor_ = executor_, .controller_delegate_ = controller_delegate_,
      .alias_resolver_ = alias_resolver_.AsStdFunction(),
      .task_manager_ = task_manager_, .session_service_ = session_service_,
      .node_event_provider_ = node_event_provider_,
      .history_service_ = history_service_,
      .monitored_item_service_ = monitored_item_service_,
      .timed_data_service_ = timed_data_service_,
      .node_service_ = node_service_, .portfolio_manager_ = portfolio_manager_,
      .local_events_ = local_events_, .favourites_ = favourites_,
      .file_cache_ = file_cache_, .profile_ = profile_,
      .dialog_service_ = dialog_service_, .blinker_manager_ = blinker_manager_,
      .create_tree_ = create_tree_, .property_service_ = property_service_,
#if !defined(UI_WT)
      .vidicon_client_ = vidicon_client_,
#endif
    };
  }
};
