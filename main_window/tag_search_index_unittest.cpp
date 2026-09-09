#include "main_window/tag_search_index.h"

#include "base/async_completion.h"
#include "base/test/test_executor.h"
#include "model/data_items_node_ids.h"
#include "node_service/test/fake_node_service.h"
#include "scada/event.h"
#include "scada/standard_node_ids.h"
#include "scada/standard_reference_types.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <optional>
#include <string>

namespace {

using testing::Contains;
using testing::IsEmpty;
using testing::Not;

class TagSearchIndexTest : public testing::Test {
 protected:
  TagSearchIndexTest() {
    // The leaf type, with no supertype, so the HasSubtype walk behind
    // IsInstanceOf stops there.
    node_service_.Add(
        scada::NodeState{.node_id = scada::data_items::id::DataItemType});
    node_service_.Add(scada::NodeState{.node_id = kRoot});
  }

  // A container the walk descends into. The fake mirrors `parent_id` onto the
  // parent's forward references, which is what GetTargets(parent, Organizes)
  // reads.
  scada::NodeId AddGroup(const scada::NodeId& parent, unsigned id) {
    const scada::NodeId group{id, 1};
    node_service_.Add(
        scada::NodeState{.node_id = group,
                         .parent_id = parent,
                         .reference_type_id = scada::id::Organizes});
    return group;
  }

  // A DataItemType leaf the walk collects as a tag.
  scada::NodeId AddTag(const scada::NodeId& parent,
                       unsigned id,
                       const std::u16string& name) {
    const scada::NodeId tag{id, 1};
    node_service_.Add(scada::NodeState{
        .node_id = tag,
        .node_class = scada::NodeClass::Variable,
        .type_definition_id = scada::data_items::id::DataItemType,
        .parent_id = parent,
        .reference_type_id = scada::id::Organizes,
        .attributes = scada::NodeAttributes{.display_name = name}});
    return tag;
  }

  // A second forward edge, for the cycle case.
  void AddBackEdge(const scada::NodeId& from, const scada::NodeId& to) {
    node_service_.Add(
        scada::NodeState{.node_id = from,
                         .references = {scada::ReferenceDescription{
                             scada::id::Organizes, true, to}}});
  }

  void Drain() {
    for (int i = 0; i < 500 && executor_.GetTaskCount() != 0; ++i)
      executor_.Poll();
  }

  std::vector<std::u16string> TagNames(const TagSearchIndex& index) {
    std::vector<std::u16string> names;
    for (const TagSearchIndex::Tag& tag : index.tags())
      names.push_back(tag.name);
    return names;
  }

  const scada::NodeId kRoot{scada::id::ObjectsFolder};

  TestExecutor executor_;
  FakeNodeService node_service_;
};

TEST_F(TagSearchIndexTest, CollectsDataItemLeavesAndNotTheContainers) {
  const scada::NodeId group = AddGroup(kRoot, 10);
  AddTag(group, 100, u"Alpha");
  AddTag(group, 101, u"Beta");

  TagSearchIndex index{executor_, node_service_, kRoot};
  index.EnsurePopulated();
  Drain();

  EXPECT_THAT(TagNames(index), Contains(u"Alpha"));
  EXPECT_THAT(TagNames(index), Contains(u"Beta"));
  EXPECT_EQ(index.tags().size(), 2u);
}

TEST_F(TagSearchIndexTest, DescendsThroughNestedContainers) {
  const scada::NodeId outer = AddGroup(kRoot, 10);
  const scada::NodeId inner = AddGroup(outer, 11);
  AddTag(inner, 100, u"Deep");

  TagSearchIndex index{executor_, node_service_, kRoot};
  index.EnsurePopulated();
  Drain();

  EXPECT_THAT(TagNames(index), Contains(u"Deep"));
}

TEST_F(TagSearchIndexTest, StopsAtMaxTags) {
  const scada::NodeId group = AddGroup(kRoot, 10);
  for (unsigned i = 0; i < 10; ++i)
    AddTag(group, 100 + i, u"Tag" + std::u16string(1, u'0' + i));

  TagSearchIndex index{executor_, node_service_, kRoot, /*max_tags=*/3};
  index.EnsurePopulated();
  Drain();

  EXPECT_EQ(index.tags().size(), 3u);
}

// The Organizes graph is not guaranteed acyclic, and the walk must not spin on
// one. `visited` is what stops it; this pins that it is still consulted.
TEST_F(TagSearchIndexTest, TerminatesOnAReferenceCycle) {
  const scada::NodeId a = AddGroup(kRoot, 10);
  const scada::NodeId b = AddGroup(a, 11);
  AddBackEdge(b, a);
  AddTag(b, 100, u"Inside");

  TagSearchIndex index{executor_, node_service_, kRoot};
  index.EnsurePopulated();
  Drain();

  EXPECT_THAT(TagNames(index), Contains(u"Inside"));
}

// The regression this file exists for. The walk used to await one node at a
// time, so a level of N containers cost N sequential round trips however fast
// the link was.
//
// Asserted on request order rather than on elapsed time: with one node's fetch
// held open, the whole level must already have been *requested*. Serially only
// the held node has been.
TEST_F(TagSearchIndexTest, IssuesALevelsFetchesBeforeAwaitingThem) {
  const scada::NodeId first = AddGroup(kRoot, 10);
  const scada::NodeId second = AddGroup(kRoot, 11);
  const scada::NodeId third = AddGroup(kRoot, 12);
  AddTag(first, 100, u"One");

  // Hold the first group's fetch open, so the walk cannot get past it by
  // completing it.
  std::optional<scada::base::AsyncCompletion> held;
  // Gate the first fetch of that node only. The walk fetches it twice -- once
  // joining it as a child of the root, once as a node of the next level -- and
  // re-emplacing would orphan the completion the walk is already suspended on.
  node_service_.SetFetchHandler(first,
                                [&](const NodeFetchStatus&) -> Awaitable<void> {
                                  if (held.has_value())
                                    co_return;
                                  held.emplace(executor_);
                                  co_await held->Wait();
                                });

  TagSearchIndex index{executor_, node_service_, kRoot};
  index.EnsurePopulated();
  Drain();

  ASSERT_TRUE(held.has_value()) << "the first group was never fetched";

  // Both siblings were requested while the first was still outstanding.
  EXPECT_THAT(node_service_.fetch_requests(second), Not(IsEmpty()));
  EXPECT_THAT(node_service_.fetch_requests(third), Not(IsEmpty()));

  held->Complete();
  Drain();

  EXPECT_THAT(TagNames(index), Contains(u"One"));
}

// Regression (backlog 720): EnsurePopulated flipped `started_` and browsed
// once; nothing reset it, cleared the tags or subscribed to the node service.
// So the palette offered deleted tags, missed created ones and showed old
// names for the whole life of the window.
TEST_F(TagSearchIndexTest, AModelChangeInvalidatesTheCollectedTags) {
  const scada::NodeId group = AddGroup(kRoot, 10);
  AddTag(group, 100, u"Alpha");

  TagSearchIndex index{executor_, node_service_, kRoot};
  index.EnsurePopulated();
  Drain();
  ASSERT_THAT(TagNames(index), Contains(u"Alpha"));

  // A node was added elsewhere; the index must not keep answering from the
  // browse that predates it.
  AddTag(group, 101, u"Beta");
  node_service_.EmitModelChanged(
      scada::ModelChangeEvent{}.set_verb(scada::ModelChangeEvent::NodeAdded));
  EXPECT_THAT(TagNames(index), IsEmpty());

  index.EnsurePopulated();
  Drain();
  EXPECT_THAT(TagNames(index), Contains(u"Alpha"));
  EXPECT_THAT(TagNames(index), Contains(u"Beta"));
}

// A rename reaches the index through the semantic-change signal, not the
// model one.
TEST_F(TagSearchIndexTest, ASemanticChangeInvalidatesTheCollectedTags) {
  const scada::NodeId group = AddGroup(kRoot, 10);
  const scada::NodeId tag = AddTag(group, 100, u"Old name");

  TagSearchIndex index{executor_, node_service_, kRoot};
  index.EnsurePopulated();
  Drain();
  ASSERT_THAT(TagNames(index), Contains(u"Old name"));

  AddTag(group, 100, u"New name");
  node_service_.EmitNodeSemanticChanged(tag);
  EXPECT_THAT(TagNames(index), IsEmpty());

  index.EnsurePopulated();
  Drain();
  EXPECT_THAT(TagNames(index), Contains(u"New name"));
  EXPECT_THAT(TagNames(index), Not(Contains(u"Old name")));
}

// Reset() is the re-login path: the window outlives a login, so the previous
// session's tags must not survive one.
TEST_F(TagSearchIndexTest, ResetDropsTheTagsAndArmsAnotherBrowse) {
  const scada::NodeId group = AddGroup(kRoot, 10);
  AddTag(group, 100, u"Alpha");

  TagSearchIndex index{executor_, node_service_, kRoot};
  index.EnsurePopulated();
  Drain();
  ASSERT_THAT(TagNames(index), Contains(u"Alpha"));

  index.Reset();
  EXPECT_THAT(TagNames(index), IsEmpty());

  index.EnsurePopulated();
  Drain();
  EXPECT_THAT(TagNames(index), Contains(u"Alpha"));
}

}  // namespace
