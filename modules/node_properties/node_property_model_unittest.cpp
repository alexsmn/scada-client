#include "modules/node_properties/node_property_model.h"

#include "address_space/generic_node_factory.h"
#include "address_space/node_utils.h"
#include "address_space/test/scada_test_address_space.h"
#include "aui/dialog_service_mock.h"
#include "base/any_executor.h"
#include "base/async_completion.h"
#include "base/check.h"
#include "base/test/awaitable_test.h"
#include "common/node_state.h"
#include "events/view_events_subscription.h"
#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "node_service/node_fetch_status.h"
#include "node_service/v3/node_fetcher.h"
#include "node_service/v3/node_service_impl.h"
#include "properties/property_context.h"
#include "properties/property_service.h"
#include "scada/attribute_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/monitored_item_service_mock.h"
#include "scada/status.h"
#include "services/task_manager_mock.h"

#include <gmock/gmock.h>

using namespace testing;

namespace {

constexpr scada::NodeId kNodeId{1, NamespaceIndexes::GROUP};

// Controllable v3::NodeFetcher: records each fetch request and suspends
// FetchNode until the test calls CompleteFetch, so tests drive the exact moment
// a node's data becomes available. On completion the NodeState is read from the
// backing address space. Replaces the former v1 AddressSpaceFetcher harness.
class ControllableNodeFetcher : public v3::NodeFetcher {
 public:
  ControllableNodeFetcher(AnyExecutor executor,
                          scada::AddressSpace& address_space)
      : executor_{std::move(executor)}, address_space_{address_space} {}

  Awaitable<scada::StatusOr<scada::NodeState>> FetchNode(
      const scada::NodeId& node_id) override {
    fetch_requests.emplace_back(node_id, NodeFetchStatus::NodeOnly);
    co_await GetGate(node_id).Wait();
    const auto* node = address_space_.GetNode(node_id);
    if (!node)
      co_return scada::StatusCode::Bad_WrongNodeId;
    co_return scada::MakeNodeState(*node);
  }

  Awaitable<scada::StatusOr<scada::ReferenceDescriptions>> FetchChildren(
      const scada::NodeId& node_id) override {
    co_return scada::ReferenceDescriptions{};
  }

  void CompleteFetch(const scada::NodeId& node_id) {
    GetGate(node_id).Complete();
  }

  std::vector<std::pair<scada::NodeId, NodeFetchStatus>> fetch_requests;

 private:
  scada::base::AsyncCompletion& GetGate(const scada::NodeId& node_id) {
    return gates_.try_emplace(node_id, executor_).first->second;
  }

  AnyExecutor executor_;
  scada::AddressSpace& address_space_;
  std::map<scada::NodeId, scada::base::AsyncCompletion> gates_;
};

class NodePropertyModelTest : public Test {
 protected:
  NodePropertyModelTest() {
    GenericNodeFactory node_factory{address_space_};
    auto [status, node] = node_factory.CreateNode(
        scada::NodeState{}
            .set_node_id(kNodeId)
            .set_node_class(scada::NodeClass::Object)
            .set_type_definition_id(data_items::id::DataGroupType)
            .set_parent(scada::id::Organizes, data_items::id::DataItems)
            .set_attributes(
                scada::NodeAttributes{}.set_display_name(u"Group")));
    base::Check(status);
    base::Check(node);

    node_service_->OnChannelOpened();
  }

  ViewEventsProvider MakeViewEventsProvider() {
    return [this](scada::ViewEvents& events)
               -> std::unique_ptr<IViewEventsSubscription> {
      view_events_ = &events;
      return std::make_unique<IViewEventsSubscription>();
    };
  }

  std::unique_ptr<NodePropertyModel> CreateModel() {
    auto model = std::make_unique<NodePropertyModel>(
        property_service_,
        PropertyContext{executor_, *node_service_, task_manager_,
                        dialog_service_},
        node_service_->GetNode(kNodeId));
    model->model_changed_handler = [this] { ++model_changed_count_; };
    model->node_deleted.connect([this] { ++node_deleted_count_; });
    return model;
  }

  // Injects a node-deleted model change through the service's view-event sink,
  // exactly as a remote server would deliver it.
  void DeleteNode(const scada::NodeId& node_id) {
    view_events_->OnModelChanged(scada::ModelChangeEvent{
        .node_id = node_id, .verb = scada::ModelChangeEvent::NodeDeleted});
  }

  aui::PropertyGroup& RootGroup(NodePropertyModel& model) {
    return static_cast<aui::PropertyModel&>(model).GetRootGroup();
  }

  scada_test::ScadaTestAddressSpace address_space_;
  TestExecutor executor_;
  NiceMock<scada::MockMonitoredItemService> monitored_item_service_;
  scada::ViewEvents* view_events_ = nullptr;
  std::shared_ptr<ControllableNodeFetcher> fetcher_ =
      std::make_shared<ControllableNodeFetcher>(executor_, address_space_);
  std::shared_ptr<v3::NodeServiceImpl> node_service_ =
      std::make_shared<v3::NodeServiceImpl>(v3::NodeServiceImplContext{
          .executor_ = executor_,
          .monitored_item_service_ = monitored_item_service_,
          .node_fetcher_ = fetcher_,
          .view_events_provider_ = MakeViewEventsProvider(),
          .keep_alive_capacity_ = 1024});
  StrictMock<MockTaskManager> task_manager_;
  StrictMock<MockDialogService> dialog_service_;
  PropertyService property_service_;
  int model_changed_count_ = 0;
  int node_deleted_count_ = 0;
};

TEST_F(NodePropertyModelTest, UpdatesAfterInitialFetchCompletes) {
  auto model = CreateModel();
  Drain(executor_);

  EXPECT_EQ(model_changed_count_, 0);
  EXPECT_EQ(RootGroup(*model).GetCount(), 0);
  // Only the target node is fetched, node-only. The exact request count is an
  // implementation detail (v3 does not dedupe fetches still in flight), so
  // assert observable behavior rather than a specific count.
  ASSERT_THAT(fetcher_->fetch_requests, Not(IsEmpty()));
  ASSERT_THAT(fetcher_->fetch_requests,
              Each(Pair(kNodeId, NodeFetchStatus::NodeOnly)));

  fetcher_->CompleteFetch(kNodeId);
  Drain(executor_);

  EXPECT_GE(model_changed_count_, 1);
  EXPECT_GT(RootGroup(*model).GetCount(), 0);
}

TEST_F(NodePropertyModelTest, DestroyedModelDoesNotUpdateAfterFetchCompletes) {
  auto model = CreateModel();
  Drain(executor_);
  model.reset();

  fetcher_->CompleteFetch(kNodeId);
  Drain(executor_);

  EXPECT_EQ(model_changed_count_, 0);
}

TEST_F(NodePropertyModelTest,
       DestroyedModelDoesNotUpdateWhenFetchCompletesBeforeCoroutineRuns) {
  auto model = CreateModel();
  model.reset();

  fetcher_->CompleteFetch(kNodeId);
  Drain(executor_);

  EXPECT_EQ(model_changed_count_, 0);
}

TEST_F(NodePropertyModelTest, DeletedNodeCancelsPendingFetchUpdate) {
  auto model = CreateModel();
  Drain(executor_);

  DeleteNode(kNodeId);
  fetcher_->CompleteFetch(kNodeId);
  Drain(executor_);

  EXPECT_EQ(node_deleted_count_, 1);
  EXPECT_EQ(model_changed_count_, 0);
  EXPECT_FALSE(model->node());
}

}  // namespace
