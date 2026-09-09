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

// Regression (backlog 719): the IOA was `ioa_start + i * ioa_step` in a bare
// int. The wizard's own spin boxes allow ioa_start 1e6, count 1e5 and
// ioa_step 1e5, so the product overflowed a signed int -- undefined
// behaviour reached by values the UI offers. Computed in 64-bit now, and a
// row past the Int32 the Address property carries is flagged rather than
// wrapped.
TEST(ExpandBulkCreateTest, IoaPastTheAddressTypeIsFlaggedNotWrapped) {
  BulkCreateParams params;
  params.name_template = u"P{n}";
  params.count = 3;
  params.ioa_start = 2000000000;
  params.ioa_step = 1000000000;

  const std::vector<BulkCreatePreviewRow> rows = ExpandBulkCreate(params, {});
  ASSERT_EQ(rows.size(), 3u);

  EXPECT_FALSE(rows[0].ioa_out_of_range);
  EXPECT_EQ(rows[0].ioa, 2000000000);
  // 3e9 and 4e9 both exceed INT32_MAX; wrapped, they would have come back
  // negative.
  EXPECT_TRUE(rows[1].ioa_out_of_range);
  EXPECT_TRUE(rows[2].ioa_out_of_range);
  EXPECT_EQ(rows[1].ioa, kMaxBulkCreateIoa);

  const BulkCreateSummary summary = SummarizeBulkCreate(rows);
  EXPECT_EQ(summary.new_count, 1);
  EXPECT_EQ(summary.out_of_range_count, 2);
}

TEST(ExpandBulkCreateTest, IoaWithinRangeIsNotFlagged) {
  BulkCreateParams params;
  params.name_template = u"P{n}";
  params.count = 3;
  params.ioa_start = 4001;
  params.ioa_step = 1;

  const std::vector<BulkCreatePreviewRow> rows = ExpandBulkCreate(params, {});
  ASSERT_EQ(rows.size(), 3u);
  EXPECT_EQ(rows[2].ioa, 4003);
  for (const BulkCreatePreviewRow& row : rows)
    EXPECT_FALSE(row.ioa_out_of_range);
}

TEST(ExpandBulkCreateTest, NonPositiveCountYieldsNoRows) {
  BulkCreateParams params;
  params.name_template = u"X{n}";
  params.count = 0;
  EXPECT_TRUE(ExpandBulkCreate(params, {}).empty());
}

}  // namespace
