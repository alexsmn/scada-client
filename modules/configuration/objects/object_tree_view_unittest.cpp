#include "configuration/objects/object_tree_view.h"

#include "address_space/test/test_scada_node_states.h"
#include "aui/qt/tree.h"
#include "aui/severity_colors.h"
#include "aui/test/app_environment.h"
#include "configuration/tree/configuration_tree_model.h"
#include "configuration/tree/configuration_tree_node.h"
#include "configuration/tree/node_service_tree_impl.h"
#include "controller/contents_model.h"
#include "controller/controller_delegate.h"
#include "controller/test/controller_environment.h"
#include "model/data_items_node_ids.h"
#include "node_service/static/static_node_service.h"
#include "profile/window_definition.h"
#include "timed_data/timed_data_service_fake.h"

#include <QLineEdit>
#include <QSortFilterProxyModel>

#include <gmock/gmock.h>

#include <memory>
#include <utility>

using namespace testing;

namespace {

// The contents of a data view, as the tree's check marks describe them. Real
// enough to answer GetContainedItems(), which is what the view re-derives every
// mark from, rather than a mock recording the calls that got it there.
class FakeContentsModel : public ContentsModel {
 public:
  void AddContainedItem(const scada::NodeId& node_id, unsigned flags) override {
    contained_.insert(node_id);
  }

  void RemoveContainedItem(const scada::NodeId& node_id) override {
    contained_.erase(node_id);
  }

  NodeIdSet GetContainedItems() const override { return contained_; }

  // Publishes `contents` the way the shell does when a data view reports its
  // whole set — SetActiveDataView's path, and the one a page restore never
  // reaches (backlog 123).
  void PublishContents(NodeIdSet contents) {
    contained_ = std::move(contents);
    NotifyContentsChanged(contained_);
  }

 private:
  NodeIdSet contained_;
};

// Owns real signals so a test can publish contents, instead of a mock whose
// Subscribe* would swallow the callback the view needs to receive.
class FakeControllerDelegate : public ControllerDelegate {
 public:
  FakeControllerDelegate() {
    contents_.contents_changed_handler = [this](const NodeIdSet& contents) {
      contents_changed_(contents);
    };
    contents_.contained_item_changed_handler =
        [this](const scada::NodeId& item_id, bool added) {
          contained_item_changed_(item_id, added);
        };
  }

  FakeContentsModel& contents() { return contents_; }

  // ControllerDelegate
  void SetTitle(std::u16string_view title) override {}
  void ShowPopupMenu(scada::aui::MenuModel* merge_menu,
                     const scada::aui::Point& point,
                     bool right_click) override {}
  void SetModified(bool modified) override {}
  void Close() override {}
  void OpenView(const WindowDefinition& def) override {}
  void ExecuteDefaultNodeCommand(const NodeRef& node) override {}
  void Focus() override {}

  ContentsModel* GetActiveContentsModel() override { return &contents_; }

  boost::signals2::scoped_connection SubscribeContentsChanged(
      const ContentsChangedCallback& callback) override {
    return contents_changed_.connect(callback);
  }

  boost::signals2::scoped_connection SubscribeContainedItemChanged(
      const ContainedItemChangedCallback& callback) override {
    return contained_item_changed_.connect(callback);
  }

 private:
  FakeContentsModel contents_;
  boost::signals2::signal<void(const NodeIdSet&)> contents_changed_;
  boost::signals2::signal<void(const scada::NodeId&, bool)>
      contained_item_changed_;
};

// Exposes the two protected accessors so a test can read the marks the view
// applied. Both are what the view itself uses; nothing here reaches past them.
class TestObjectTreeView : public ObjectTreeView {
 public:
  using ConfigurationTreeView::model;
  using ConfigurationTreeView::tree_view;
  using ObjectTreeView::ObjectTreeView;
};

}  // namespace

// The marks are seeded when a node materializes, not only when contents
// change. A restored page publishes its contents before the lazily built tree
// holds any of the nodes they name, so a view that only reacts to the
// contents-changed notification leaves every box clear while the table lists
// exactly those items (client e7de1bdb8, which shipped without this test —
// backlog 124).
class ObjectTreeViewTest : public Test {
 protected:
  ObjectTreeViewTest()
      : node_service_tree_factory_{[](NodeServiceTreeImplContext&& context) {
          return std::make_unique<NodeServiceTreeImpl>(std::move(context));
        }} {}

  void SetUp() override {
    node_service_.AddAll(GetScadaNodeStates());

    node_service_.Add(scada::NodeState{
        .node_id = kGroupId,
        .node_class = scada::NodeClass::Object,
        .type_definition_id = scada::data_items::id::DataGroupType,
        .parent_id = scada::data_items::id::DataItems,
        .reference_type_id = scada::id::Organizes});

    for (const scada::NodeId& item_id : {kItem1Id, kItem2Id}) {
      node_service_.Add(scada::NodeState{
          .node_id = item_id,
          .node_class = scada::NodeClass::Variable,
          .type_definition_id = scada::data_items::id::DataItemType,
          .parent_id = kGroupId,
          .reference_type_id = scada::id::Organizes});
    }

    // A data item directly under the tree's root, i.e. a top-level row. Its
    // live value subscription is the one that depended on the root being
    // treated as showing its children (V45).
    node_service_.Add(scada::NodeState{
        .node_id = kTopLevelItemId,
        .node_class = scada::NodeClass::Variable,
        .type_definition_id = scada::data_items::id::DataItemType,
        .parent_id = scada::data_items::id::DataItems,
        .reference_type_id = scada::id::Organizes});

    // A three-level branch, kept separate from kGroupId so the container-mark
    // tests still see a group whose children are exactly their two items.
    //
    // Depth is the point. Subscribing a top-level row fetches it, and fetching
    // a node materializes its children, so everything one level below the root
    // now exists as soon as the tree loads. kDeepItemId sits two levels down,
    // under a nested group that nothing subscribes, so it is still absent when
    // contents are published -- which is the state
    // NodesMaterializedAfterContentsComeUpMarked has to start from.
    node_service_.Add(scada::NodeState{
        .node_id = kDeepGroupId,
        .node_class = scada::NodeClass::Object,
        .type_definition_id = scada::data_items::id::DataGroupType,
        .parent_id = scada::data_items::id::DataItems,
        .reference_type_id = scada::id::Organizes});
    node_service_.Add(scada::NodeState{
        .node_id = kNestedGroupId,
        .node_class = scada::NodeClass::Object,
        .type_definition_id = scada::data_items::id::DataGroupType,
        .parent_id = kDeepGroupId,
        .reference_type_id = scada::id::Organizes});
    node_service_.Add(scada::NodeState{
        .node_id = kDeepItemId,
        .node_class = scada::NodeClass::Variable,
        .type_definition_id = scada::data_items::id::DataItemType,
        .parent_id = kNestedGroupId,
        .reference_type_id = scada::id::Organizes});

    view_ = std::make_unique<TestObjectTreeView>(MakeContext(),
                                                 node_service_tree_factory_);
    ui_view_ = view_->Init(WindowDefinition{});
    ASSERT_THAT(ui_view_, NotNull());
  }

  ControllerContext MakeContext() {
    return {.executor_ = env_.executor_,
            .controller_delegate_ = delegate_,
            .task_manager_ = env_.task_manager_,
            .session_service_ = env_.session_service_,
            .node_event_provider_ = env_.node_event_provider_,
            .history_service_ = env_.history_service_,
            .monitored_item_service_ = env_.monitored_item_service_,
            .timed_data_service_ = timed_data_service_,
            .node_service_ = node_service_,
            .attribute_service_ = env_.attribute_service_,
            .file_cache_ = env_.file_cache_,
            .profile_ = env_.profile_,
            .dialog_service_ = env_.dialog_service_,
            .blinker_manager_ = env_.blinker_manager_,
            .create_tree_ = env_.create_tree_,
            .property_service_ = env_.property_service_,
            .frame_capture_registry_ = env_.frame_capture_registry_,
            .display_selection_registry_ = env_.display_selection_registry_};
  }

  // Materializes one level below `node`. The tree is lazy, so until this runs
  // the children simply do not exist — which is the state a restored page
  // publishes its contents into.
  void Materialize(ConfigurationTreeNode& node) {
    if (node.CanFetchMore())
      node.FetchMore();
    env_.executor_.Poll();
  }

  void MaterializeWholeTree() {
    ASSERT_THAT(view_->model().root(), NotNull());
    Materialize(*view_->model().root());
    ConfigurationTreeNode* group = view_->model().FindFirstTreeNode(kGroupId);
    ASSERT_THAT(group, NotNull());
    Materialize(*group);
  }

  // Materializes the three-level branch down to kDeepItemId.
  void MaterializeDeepBranch() {
    ASSERT_THAT(view_->model().root(), NotNull());
    Materialize(*view_->model().root());
    ConfigurationTreeNode* deep =
        view_->model().FindFirstTreeNode(kDeepGroupId);
    ASSERT_THAT(deep, NotNull());
    Materialize(*deep);
    ConfigurationTreeNode* nested =
        view_->model().FindFirstTreeNode(kNestedGroupId);
    ASSERT_THAT(nested, NotNull());
    Materialize(*nested);
  }

  bool IsCheckedById(const scada::NodeId& node_id) {
    ConfigurationTreeNode* node = view_->model().FindFirstTreeNode(node_id);
    EXPECT_THAT(node, NotNull()) << "node " << node_id.ToString();
    return node && view_->tree_view().IsChecked(node);
  }

  // Ticks a row's box the way an operator's click does: through the view's own
  // item model, with `Qt::CheckStateRole`.
  //
  // `Tree::SetChecked` is the programmatic setter the view uses to *apply* a
  // mark and deliberately does not run `Tree::SetCheckedHandler`'s handler, so
  // the whole handler body -- including the `GetOrderedNodes` walk backlog 125
  // is about -- was unreachable from a test while `SetChecked` was the only way
  // in (backlog 124). Nothing had to be added to `Tree` for this: it is a
  // `QTreeView`, so its proxy model is public, and `mapToSource` recovers the
  // node pointer the adapter stored in the index.
  void ClickCheckBox(const scada::NodeId& node_id, bool checked) {
    QModelIndex index = IndexOf(node_id);
    ASSERT_TRUE(index.isValid()) << "no row for " << node_id.ToString();
    ASSERT_TRUE(view_->tree_view().model()->setData(
        index, checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole));
  }

  QModelIndex IndexOf(const scada::NodeId& node_id) {
    ConfigurationTreeNode* node = view_->model().FindFirstTreeNode(node_id);
    EXPECT_THAT(node, NotNull()) << "node " << node_id.ToString();
    auto* proxy =
        qobject_cast<QSortFilterProxyModel*>(view_->tree_view().model());
    EXPECT_THAT(proxy, NotNull());
    return node && proxy ? FindNode(*proxy, QModelIndex{}, node)
                         : QModelIndex{};
  }

  static QModelIndex FindNode(QSortFilterProxyModel& proxy,
                              const QModelIndex& parent,
                              const void* node) {
    for (int row = 0; row < proxy.rowCount(parent); ++row) {
      QModelIndex index = proxy.index(row, 0, parent);
      if (proxy.mapToSource(index).internalPointer() == node)
        return index;
      QModelIndex found = FindNode(proxy, index, node);
      if (found.isValid())
        return found;
    }
    return {};
  }

  // Whether the row holds a live-value subscription.
  //
  // Reads the Value column's TEXT COLOUR, which is the one accessor that
  // separates the two states this test has to tell apart: VisibleNodeModel
  // returns `bad_value_color` when the row has no VisibleNode at all, and
  // Transparent when it has one that is not bad -- and a ProxyVisibleNode
  // whose fetch has not landed reports IsBad() == false, so it reads as
  // subscribed. GetStatusColor cannot be used here: it answers nullopt for
  // *both* an absent node and an unresolved one, which is what made an earlier
  // version of this test unable to distinguish a broken fix from a slow one.
  //
  // Deliberately not asserting the value or the dot. Those need the async
  // node/type-chain fetch to complete, which is ObjectTreeModel's contract and
  // is covered by ObjectTreeModelAsyncVisibleNodeTest; this fixture's subject
  // is only whether the row gets subscribed in the first place.
  bool IsSubscribed(const scada::NodeId& node_id) {
    ConfigurationTreeNode* node = view_->model().FindFirstTreeNode(node_id);
    EXPECT_THAT(node, NotNull()) << "node " << node_id.ToString();
    if (!node)
      return false;
    return view_->model().GetTextColor(node, 1) !=
           env_.profile_.bad_value_color;
  }

  static inline const scada::NodeId kGroupId{2001, 1};
  static inline const scada::NodeId kTopLevelItemId{2004, 1};
  static inline const scada::NodeId kDeepGroupId{2005, 1};
  static inline const scada::NodeId kNestedGroupId{2006, 1};
  static inline const scada::NodeId kDeepItemId{2007, 1};
  static inline const scada::NodeId kItem1Id{2002, 1};
  static inline const scada::NodeId kItem2Id{2003, 1};

  AppEnvironment app_env_;
  ControllerEnvironment env_;
  FakeControllerDelegate delegate_;
  StaticNodeService node_service_;
  FakeTimedDataService timed_data_service_;
  NodeServiceTreeFactory node_service_tree_factory_;
  std::unique_ptr<TestObjectTreeView> view_;
  std::unique_ptr<UiView> ui_view_;
};

// The regression. Contents arrive first — the page restore case — and the rows
// they name are created afterwards. Before the fix nothing reconciled a node
// that appeared later, so both items came up clear.
TEST_F(ObjectTreeViewTest, NodesMaterializedAfterContentsComeUpMarked) {
  delegate_.contents().PublishContents(NodeIdSet{kDeepItemId});

  // The precondition IS the proof: with nothing materialized, the publish
  // above cannot have marked anything, so a mark that appears later can only
  // have been seeded on the way in.
  //
  // It moved two levels down on 2026-09-19. It used to name an item under
  // kGroupId, and that stopped being unmaterialized when top-level rows began
  // subscribing to their values (V45): subscribing a row fetches it, and
  // fetching a node materializes its children, so everything one level below
  // the root now exists as soon as the tree loads. Relaxing the assertion
  // would have kept this test green while retiring what it proves -- the mark
  // could then have come from the publish rather than from the seeding.
  ASSERT_THAT(view_->model().FindFirstTreeNode(kDeepItemId), IsNull());

  MaterializeDeepBranch();

  EXPECT_TRUE(IsCheckedById(kDeepItemId));
}

// The V45 regression. A data item at the top level of the tree subscribes to
// its live value, so its Value column and quality dot are populated.
//
// Before the fix nothing subscribed it: OnTreeNodesAdded gated on
// `IsExpanded(parent)`, and the root is never drawn as a row, so the view holds
// no expansion state for it and that answered false forever. Every top-level
// data item therefore showed a permanently empty Value column — in the shipped
// client, not only in the screenshot gallery where it was noticed. Rows under
// an expanded parent were always fine, which is why it survived: the same
// capture showed both, and only the top-level half was wrong.
TEST_F(ObjectTreeViewTest, ATopLevelDataItemSubscribesToItsValue) {
  EXPECT_TRUE(IsSubscribed(kTopLevelItemId));
}

// The other half of the same rule, and the reason the bug was invisible: a row
// whose parent is collapsed must NOT subscribe. Without this, "fix" the first
// test by subscribing everything and nothing would notice.
TEST_F(ObjectTreeViewTest, AnItemUnderACollapsedParentDoesNotSubscribe) {
  MaterializeWholeTree();

  ASSERT_FALSE(view_->tree_view().IsExpanded(
      view_->model().FindFirstTreeNode(kGroupId), false));
  EXPECT_FALSE(IsSubscribed(kItem1Id));
}

// A container is marked exactly when everything under it is, and that rule has
// to survive the same ordering: the group's own mark is derived from children
// that did not exist when the contents were published.
TEST_F(ObjectTreeViewTest, AContainerWhoseItemsAreAllContainedComesUpMarked) {
  delegate_.contents().PublishContents(NodeIdSet{kItem1Id, kItem2Id});

  MaterializeWholeTree();

  EXPECT_TRUE(IsCheckedById(kGroupId));
}

// The other half of that rule, and the one an over-eager seeding would break:
// a container holding an item nobody asked for is not marked.
TEST_F(ObjectTreeViewTest, AContainerWithAnUncontainedItemStaysClear) {
  delegate_.contents().PublishContents(NodeIdSet{kItem1Id});

  MaterializeWholeTree();

  EXPECT_TRUE(IsCheckedById(kItem1Id));
  EXPECT_FALSE(IsCheckedById(kItem2Id));
  EXPECT_FALSE(IsCheckedById(kGroupId));
}

// With no contents at all every box is clear — the seeding must not invent a
// mark for a node it knows nothing about. Without this the tests above pass
// against a view that simply marks everything.
TEST_F(ObjectTreeViewTest, NodesMaterializedWithNoContentsStayClear) {
  MaterializeWholeTree();

  EXPECT_FALSE(IsCheckedById(kItem1Id));
  EXPECT_FALSE(IsCheckedById(kItem2Id));
  EXPECT_FALSE(IsCheckedById(kGroupId));
}

// The already-materialized direction still works: contents published against a
// tree that is fully built mark it through SetContents rather than through the
// nodes-added seam.
TEST_F(ObjectTreeViewTest, ContentsPublishedAfterTheTreeIsBuiltMarkIt) {
  MaterializeWholeTree();
  ASSERT_FALSE(IsCheckedById(kItem1Id));

  delegate_.contents().PublishContents(NodeIdSet{kItem1Id, kItem2Id});

  EXPECT_TRUE(IsCheckedById(kItem1Id));
  EXPECT_TRUE(IsCheckedById(kItem2Id));
  EXPECT_TRUE(IsCheckedById(kGroupId));
}

// The click, which nothing exercised until the seam above existed. Ticking a
// row's box is what puts the item into the active view -- the whole point of
// the checkable tree -- and it goes through the handler, not through
// `Tree::SetChecked`.
TEST_F(ObjectTreeViewTest, CheckingAnItemAddsItToTheContents) {
  MaterializeWholeTree();
  ASSERT_THAT(delegate_.contents().GetContainedItems(), IsEmpty());

  ClickCheckBox(kItem1Id, true);

  EXPECT_THAT(delegate_.contents().GetContainedItems(),
              UnorderedElementsAre(kItem1Id));
  EXPECT_TRUE(IsCheckedById(kItem1Id));
}

// The assertion backlog 124 asked for. The handler re-derives every mark from
// what the view holds after the add, and `AddContainedItem` on an item already
// there is a no-op -- so a second click must leave the contents exactly as they
// were rather than duplicating the item or clearing the mark.
TEST_F(ObjectTreeViewTest, ReCheckingAContainedItemLeavesTheContentsAlone) {
  MaterializeWholeTree();
  ClickCheckBox(kItem1Id, true);
  const NodeIdSet after_first = delegate_.contents().GetContainedItems();

  ClickCheckBox(kItem1Id, true);

  EXPECT_EQ(after_first, delegate_.contents().GetContainedItems());
  EXPECT_TRUE(IsCheckedById(kItem1Id));
}

// Unchecking is the other half of the same handler, and it must take the item
// back out rather than only clearing the box.
TEST_F(ObjectTreeViewTest, UncheckingAContainedItemRemovesItFromTheContents) {
  MaterializeWholeTree();
  ClickCheckBox(kItem1Id, true);
  ASSERT_TRUE(IsCheckedById(kItem1Id));

  ClickCheckBox(kItem1Id, false);

  EXPECT_THAT(delegate_.contents().GetContainedItems(), IsEmpty());
  EXPECT_FALSE(IsCheckedById(kItem1Id));
}

// Checking a container adds everything under it, in tree order -- the
// `GetOrderedNodes` walk. This is the materialized case; the collapsed one is
// backlog 125, and it is still broken.
TEST_F(ObjectTreeViewTest, CheckingAMaterializedGroupAddsEveryItemUnderIt) {
  MaterializeWholeTree();

  ClickCheckBox(kGroupId, true);

  EXPECT_THAT(delegate_.contents().GetContainedItems(),
              UnorderedElementsAre(kItem1Id, kItem2Id));
  EXPECT_TRUE(IsCheckedById(kGroupId));
}

// The Explorer's filter field.
class ObjectTreeViewFilterTest : public ObjectTreeViewTest {
 protected:
  QLineEdit* Filter() const {
    return ui_view_->findChild<QLineEdit*>(QStringLiteral("explorerFilter"));
  }
};

// An empty filter changes nothing about what the operator sees, so the field
// must not carry a frame while it is unused -- it was a filled, outlined pill
// at full contrast, the loudest thing in the pane. The frame is the signal
// that the field is doing something.
TEST_F(ObjectTreeViewFilterTest, FilterDrawsNoFrameUntilItHasText) {
  QLineEdit* filter = Filter();
  ASSERT_THAT(filter, NotNull());

  EXPECT_FALSE(filter->hasFrame());

  filter->setText(QStringLiteral("КРУ"));
  EXPECT_TRUE(filter->hasFrame());

  filter->clear();
  EXPECT_FALSE(filter->hasFrame());
}

// Its height must not move when the frame appears, or the tree under it jumps
// as the operator types the first character.
TEST_F(ObjectTreeViewFilterTest, FilterHeightIsStableAcrossTheFrame) {
  QLineEdit* filter = Filter();
  ASSERT_THAT(filter, NotNull());

  const int unframed = filter->height();
  filter->setText(QStringLiteral("x"));
  EXPECT_EQ(filter->height(), unframed);
}

// The field is the platform's, not a transcription of the mockup's CSS. A
// stylesheet here is the regression this replaced: baked token colours, a
// hand-tuned radius and px padding, none of which follow the host theme or the
// OS font-size setting (docs/client/ux/README.md, native direction).
TEST_F(ObjectTreeViewFilterTest, FilterCarriesNoStyleSheet) {
  QLineEdit* filter = Filter();
  ASSERT_THAT(filter, NotNull());

  EXPECT_TRUE(filter->styleSheet().isEmpty());
}
