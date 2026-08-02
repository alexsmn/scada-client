#include "bulk_create/bulk_create_pattern.h"

#include <gtest/gtest.h>

#include <set>

namespace {

TEST(ExpandTokensTest, DecimalIndexToken) {
  EXPECT_EQ(ExpandTokens(u"TS{n} current", 8), u"TS8 current");
  EXPECT_EQ(ExpandTokens(u"TS{n} current", 24), u"TS24 current");
}

TEST(ExpandTokensTest, ZeroPaddedToken) {
  EXPECT_EQ(ExpandTokens(u"AI{nn}", 3), u"AI03");
  EXPECT_EQ(ExpandTokens(u"AI{nn}", 24), u"AI24");
  EXPECT_EQ(ExpandTokens(u"AI{nn}", 100), u"AI100");
}

TEST(ExpandTokensTest, HexToken) {
  EXPECT_EQ(ExpandTokens(u"reg_{hex}", 10), u"reg_a");
  EXPECT_EQ(ExpandTokens(u"reg_{hex}", 255), u"reg_ff");
}

TEST(ExpandTokensTest, MultipleTokensAndVerbatimText) {
  EXPECT_EQ(ExpandTokens(u"ns=2;s=RTU.TS{n}.I", 5), u"ns=2;s=RTU.TS5.I");
}

TEST(ExpandTokensTest, UnknownTokenLeftUntouched) {
  EXPECT_EQ(ExpandTokens(u"a{x}b", 5), u"a{x}b");
}

TEST(ExpandBulkCreateTest, ExpandsCountRowsWithSteppedIndexAndIoa) {
  BulkCreateParams params;
  params.name_template = u"TS{n}";
  params.node_id_template = u"ns=2;s=RTU.TS{n}";
  params.start_index = 1;
  params.count = 3;
  params.index_step = 1;
  params.ioa_start = 4001;
  params.ioa_step = 1;

  const std::vector<BulkCreatePreviewRow> rows = ExpandBulkCreate(params, {});
  ASSERT_EQ(rows.size(), 3u);
  EXPECT_EQ(rows[0].name, u"TS1");
  EXPECT_EQ(rows[0].node_id, u"ns=2;s=RTU.TS1");
  EXPECT_EQ(rows[0].ioa, 4001);
  EXPECT_EQ(rows[2].name, u"TS3");
  EXPECT_EQ(rows[2].ioa, 4003);
  EXPECT_FALSE(rows[0].conflict);
}

TEST(ExpandBulkCreateTest, MarksConflictsAgainstExistingNodeIds) {
  BulkCreateParams params;
  params.name_template = u"TS{n}";
  params.node_id_template = u"ns=2;s=RTU.TS{n}.I";
  params.start_index = 1;
  params.count = 10;

  const std::set<std::u16string> existing{u"ns=2;s=RTU.TS8.I"};
  const std::vector<BulkCreatePreviewRow> rows =
      ExpandBulkCreate(params, existing);
  ASSERT_EQ(rows.size(), 10u);
  EXPECT_TRUE(rows[7].conflict);  // TS8
  EXPECT_FALSE(rows[0].conflict);

  const BulkCreateSummary summary = SummarizeBulkCreate(rows);
  EXPECT_EQ(summary.new_count, 9);
  EXPECT_EQ(summary.conflict_count, 1);
}

TEST(ExpandBulkCreateTest, IndexStepAdvancesTheIndex) {
  BulkCreateParams params;
  params.name_template = u"P{n}";
  params.start_index = 10;
  params.count = 3;
  params.index_step = 5;

  const std::vector<BulkCreatePreviewRow> rows = ExpandBulkCreate(params, {});
  ASSERT_EQ(rows.size(), 3u);
  EXPECT_EQ(rows[0].name, u"P10");
  EXPECT_EQ(rows[1].name, u"P15");
  EXPECT_EQ(rows[2].name, u"P20");
}

TEST(ExpandBulkCreateTest, NonPositiveCountYieldsNoRows) {
  BulkCreateParams params;
  params.name_template = u"X{n}";
  params.count = 0;
  EXPECT_TRUE(ExpandBulkCreate(params, {}).empty());
}

}  // namespace
