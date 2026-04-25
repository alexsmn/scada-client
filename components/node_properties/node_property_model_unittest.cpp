#include "components/node_properties/node_property_model.h"

#include "address_space/address_space_impl3.h"
#include "address_space/generic_node_factory.h"
#include "aui/dialog_service_mock.h"
#include "base/test/awaitable_test.h"
#include "common/node_state.h"
#include "model/data_items_node_ids.h"
#include "model/namespaces.h"
#include "node_service/node_fetch_status.h"
#include "node_service/v1/address_space_fetcher.h"
#include "node_service/v1/address_space_fetcher_factory.h"
#include "node_service/v1/node_fetch_status_types.h"
#include "node_service/v1/node_service_impl.h"
#include "properties/property_context.h"
#include "properties/property_service.h"
#include "scada/attribute_service_mock.h"
#include "scada/method_service_mock.h"
#include "scada/monitored_item_service_mock.h"
#include "scada/status.h"
#include "services/task_manager_mock.h"

#include <gmock/gmock.h>

#include <cassert>

using namespace testing;

namespace {

constexpr scada::NodeId kNodeId{1, NamespaceIndexes::GROUP};

class ControllableAddressSpaceFetcher : public v1::AddressSpaceFetcher {
 public:
  void Init(v1::AddressSpaceFetcherFactoryContext&& context) {
    node_fetch_status_changed_handler_ =
        std::move(context.node_fetch_status_changed_handler_);
    model_changed_handler_ = std::move(context.model_changed_handler_);
    semantic_changed_handler_ = std::move(context.semantic_changed_handler_);
  }

  void OnChannelOpened() override {}
  void OnChannelClosed() override {}

  std::pair<scada::Status, NodeFetchStatus> GetNodeFetchStatus(
      const scada::NodeId& node_id) const override {
    if (auto i = fetch_statuses_.find(node_id); i != fetch_statuses_.end()) {
      return {scada::StatusCode::Good, i->second};
    }
    return {scada::StatusCode::Good, NodeFetchStatus::Max()};
  }

  void FetchNode(const scada::NodeId& node_id,
                 const NodeFetchStatus& requested_status) override {
    fetch_requests.emplace_back(node_id, requested_status);
    ++pending_task_count_;
  }

  size_t GetPendingTaskCount() const override { return pending_task_count_; }

  void SetFetchStatus(const scada::NodeId& node_id,
                      NodeFetchStatus fetch_status) {
    fetch_statuses_[node_id] = fetch_status;
  }

  void CompleteFetch(const scada::NodeId& node_id,
                     NodeFetchStatus fetch_status = NodeFetchStatus::Max()) {
    SetFetchStatus(node_id, fetch_status);
    if (pending_task_count_ > 0) {
      --pending_task_count_;
    }

    v1::NodeFetchStatusChangedItem item{.node_id = node_id,
                                        .status = scada::StatusCode::Good,
                                        .fetch_status = fetch_status};
    node_fetch_status_changed_handler_(std::span{&item, 1});
  }

  void DeleteNode(const scada::NodeId& node_id) {
    model_changed_handler_(scada::ModelChangeEvent{
        .node_id = node_id,
        .verb = scada::ModelChangeEvent::NodeDeleted});
  }

  std::vector<std::pair<scada::NodeId, NodeFetchStatus>> fetch_requests;

 private:
  v1::NodeFetchStatusChangedHandler node_fetch_status_changed_handler_;
  std::function<void(const scada::ModelChangeEvent& event)>
      model_changed_handler_;
  std::function<void(const scada::SemanticChangeEvent& event)>
      semantic_changed_handler_;
  std::map<scada::NodeId, NodeFetchStatus> fetch_statuses_;
  size_t pending_task_count_ = 0;
};

class NodePropertyModelTest : public Test {
 protected:
  NodePropertyModelTest()
      : node_service_{v1::NodeServiceImplContext{
            MakeAddressSpaceFetcherFactory(), address_space_,
            attribute_service_, monitored_item_service_, method_service_}} {
    GenericNodeFactory node_factory{address_space_};
    auto [status, node] = node_factory.CreateNode(
        scada::NodeState{}
            .set_node_id(kNodeId)
            .set_node_class(scada::NodeClass::Object)
            .set_type_definition_id(data_items::id::DataGroupType)
            .set_parent(scada::id::Organizes, data_items::id::DataItems)
            .set_attributes(
                scada::NodeAttributes{}.set_display_name(u"Group")));
    assert(status);
    assert(node);

    fetcher_->SetFetchStatus(kNodeId, NodeFetchStatus{});
  }

  v1::AddressSpaceFetcherFactory MakeAddressSpaceFetcherFactory() {
    return [fetcher = fetcher_](
               v1::AddressSpaceFetcherFactoryContext&& context) {
      fetcher->Init(std::move(context));
      return fetcher;
    };
  }

  std::unique_ptr<NodePropertyModel> CreateModel() {
    auto model = std::make_unique<NodePropertyModel>(
        property_service_,
        PropertyContext{executor_, node_service_, task_manager_,
                        dialog_service_},
        node_service_.GetNode(kNodeId));
    model->model_changed_handler = [this] { ++model_changed_count_; };
    model->node_deleted.connect([this] { ++node_deleted_count_; });
    return model;
  }

  aui::PropertyGroup& RootGroup(NodePropertyModel& model) {
    return static_cast<aui::PropertyModel&>(model).GetRootGroup();
  }

  std::shared_ptr<ControllableAddressSpaceFetcher> fetcher_ =
      std::make_shared<ControllableAddressSpaceFetcher>();
  AddressSpaceImpl3 address_space_;
  NiceMock<scada::MockAttributeService> attribute_service_;
  NiceMock<scada::MockMonitoredItemService> monitored_item_service_;
  NiceMock<scada::MockMethodService> method_service_;
  v1::NodeServiceImpl node_service_;
  std::shared_ptr<TestExecutor> executor_ = std::make_shared<TestExecutor>();
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
  ASSERT_THAT(fetcher_->fetch_requests,
              ElementsAre(Pair(kNodeId, NodeFetchStatus::NodeOnly())));

  fetcher_->CompleteFetch(kNodeId);
  Drain(executor_);

  EXPECT_EQ(model_changed_count_, 1);
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

TEST_F(NodePropertyModelTest, DeletedNodeCancelsPendingFetchUpdate) {
  auto model = CreateModel();
  Drain(executor_);

  fetcher_->DeleteNode(kNodeId);
  fetcher_->CompleteFetch(kNodeId);
  Drain(executor_);

  EXPECT_EQ(node_deleted_count_, 1);
  EXPECT_EQ(model_changed_count_, 0);
  EXPECT_FALSE(model->node());
}

}  // namespace
