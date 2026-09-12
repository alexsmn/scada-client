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
  // These assert on the IOA, so they are the transmission branch: only it
  // addresses its rows on a link.
  params.subject = BulkCreateSubject::kTransmissionItem;
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
  params.subject = BulkCreateSubject::kTransmissionItem;
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
  params.subject = BulkCreateSubject::kTransmissionItem;
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

// One wizard, two subjects. The subject decides only the binding and the
// address, which is what lets a single dialog create both.
TEST(BulkCreateSubjectTest, OnlyTheTransmissionBranchAddressesOnALink) {
  EXPECT_FALSE(BulkCreateSubjectUsesIoa(BulkCreateSubject::kDataItem));
  EXPECT_TRUE(BulkCreateSubjectUsesIoa(BulkCreateSubject::kTransmissionItem));
}

// A data item is not addressed on a link, so its rows carry no IOA at all.
// Absent rather than zero: zero is a real address the grid would render and
// the range check would call in-range, so collapsing the two would make every
// data-item row claim IOA 0.
TEST(ExpandBulkCreateTest, DataItemRowsCarryNoIoa) {
  BulkCreateParams params;
  params.subject = BulkCreateSubject::kDataItem;
  params.name_template = u"AI{n}";
  params.node_id_template = u"ns=2;s=RTU.AI{n}";
  params.count = 3;
  // Deliberately set, and deliberately ignored: the fields are shared by both
  // subjects and a stale value from a previous subject must not leak into a
  // data item.
  params.ioa_start = 4001;
  params.ioa_step = 1;

  const std::vector<BulkCreatePreviewRow> rows = ExpandBulkCreate(params, {});
  ASSERT_EQ(rows.size(), 3u);
  for (const BulkCreatePreviewRow& row : rows) {
    EXPECT_TRUE(row.ioa_absent);
    EXPECT_FALSE(row.ioa_out_of_range);
    EXPECT_EQ(row.ioa, 0);
  }
}

// Switching subject must change nothing but the address half: the names, the
// ids and the conflict flags are the subject-independent part, and that is the
// whole reason one dialog can serve both.
TEST(ExpandBulkCreateTest, SubjectChangesTheAddressHalfAndNothingElse) {
  BulkCreateParams params;
  params.name_template = u"X{n}";
  params.node_id_template = u"ns=2;s=X{n}";
  params.count = 4;
  params.ioa_start = 10;
  params.ioa_step = 2;
  const std::set<std::u16string> existing{u"ns=2;s=X2"};

  params.subject = BulkCreateSubject::kDataItem;
  const std::vector<BulkCreatePreviewRow> data_rows =
      ExpandBulkCreate(params, existing);
  params.subject = BulkCreateSubject::kTransmissionItem;
  const std::vector<BulkCreatePreviewRow> tx_rows =
      ExpandBulkCreate(params, existing);

  ASSERT_EQ(data_rows.size(), tx_rows.size());
  for (std::size_t i = 0; i < data_rows.size(); ++i) {
    EXPECT_EQ(data_rows[i].number, tx_rows[i].number);
    EXPECT_EQ(data_rows[i].name, tx_rows[i].name);
    EXPECT_EQ(data_rows[i].node_id, tx_rows[i].node_id);
    EXPECT_EQ(data_rows[i].conflict, tx_rows[i].conflict);
  }
  EXPECT_TRUE(data_rows[0].ioa_absent);
  EXPECT_FALSE(tx_rows[0].ioa_absent);
  EXPECT_EQ(tx_rows[3].ioa, 16);
}

// The summary counts a data item's absent address as neither a conflict nor
// out of range, so a data-item run reads "N new" exactly as before.
TEST(SummarizeBulkCreateTest, DataItemRowsCountAsNew) {
  BulkCreateParams params;
  params.subject = BulkCreateSubject::kDataItem;
  params.name_template = u"AI{n}";
  params.node_id_template = u"ns=2;s=AI{n}";
  params.count = 5;

  const BulkCreateSummary summary =
      SummarizeBulkCreate(ExpandBulkCreate(params, {u"ns=2;s=AI3"}));
  EXPECT_EQ(summary.new_count, 4);
  EXPECT_EQ(summary.conflict_count, 1);
  EXPECT_EQ(summary.out_of_range_count, 0);
}

}  // namespace
