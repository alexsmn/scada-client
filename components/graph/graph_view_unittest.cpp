#include "components/graph/graph_view.h"

#include "base/blinker_mock.h"
#include "base/test/test_executor.h"
#include "common/aliases_mock.h"
#include "common/node_event_provider_mock.h"
#include "common_resources.h"
#include "components/favourites/favourites.h"
#include "components/portfolio/portfolio_manager.h"
#include "controller_delegate_mock.h"
#include "core/history_service_mock.h"
#include "core/monitored_item_service_mock.h"
#include "core/session_service_mock.h"
#include "node_service/node_service_mock.h"
#include "services/create_tree.h"
#include "services/dialog_service_mock.h"
#include "services/file_cache.h"
#include "services/file_registry.h"
#include "services/local_events.h"
#include "services/profile.h"
#include "services/task_manager_mock.h"
#include "timed_data/timed_data_service_mock.h"
#include "window_definition.h"
#include "window_info.h"

#include <gmock/gmock.h>

using namespace testing;

class GraphViewTest : public Test {
 public:
  virtual void SetUp() override;

 protected:
  const std::shared_ptr<TestExecutor> executor_ =
      std::make_shared<TestExecutor>();
  NiceMock<MockControllerDelegate> controller_delegate_;
  NiceMock<MockAliasResolver> alias_resolver_;
  NiceMock<MockTaskManager> task_manager_;
  NiceMock<scada::MockSessionService> session_service_;
  NiceMock<MockNodeEventProvider> node_event_provider_;
  NiceMock<scada::MockHistoryService> history_service_;
  NiceMock<scada::MockMonitoredItemService> monitored_item_service_;
  NiceMock<MockTimedDataService> timed_data_service_;
  NiceMock<MockNodeService> node_service_;
  PortfolioManager portfolio_manager_{{.node_service_ = node_service_}};
  LocalEvents local_events_;
  Favourites favourites_;
  FileCache file_cache_{FileRegistry{}};
  Profile profile_;
  NiceMock<MockDialogService> dialog_service_;
  NiceMock<MockBlinkerManager> blinker_manager_;
  CreateTree create_tree_;

  GraphView graph_view_{
      ControllerContext{.executor_ = executor_,
                        .controller_delegate_ = controller_delegate_,
                        .alias_resolver_ = alias_resolver_.AsStdFunction(),
                        .task_manager_ = task_manager_,
                        .session_service_ = session_service_,
                        .node_event_provider_ = node_event_provider_,
                        .history_service_ = history_service_,
                        .monitored_item_service_ = monitored_item_service_,
                        .timed_data_service_ = timed_data_service_,
                        .node_service_ = node_service_,
                        .portfolio_manager_ = portfolio_manager_,
                        .local_events_ = local_events_,
                        .favourites_ = favourites_,
                        .file_cache_ = file_cache_,
                        .profile_ = profile_,
                        .dialog_service_ = dialog_service_,
                        .blinker_manager_ = blinker_manager_,
                        .create_tree_ = create_tree_}};

  std::shared_ptr<UiView> ui_view_;
};

void GraphViewTest::SetUp() {
  WindowDefinition def{GetWindowInfo(ID_GRAPH_VIEW)};
  ui_view_.reset(graph_view_.Init(def));
}

TEST_F(GraphViewTest, Test) {}
