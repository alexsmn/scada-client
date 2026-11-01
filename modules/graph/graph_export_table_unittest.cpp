#include "graph/graph_export_table.h"

#include "base/format_time.h"
#include "base/utf_convert.h"

#include <gmock/gmock.h>

using namespace std::chrono_literals;
using namespace testing;

namespace {

const scada::Time kStart = scada::Time{} + 1000h;

scada::DataValue Sample(double value, scada::Duration offset) {
  return {scada::Variant{value}, {}, kStart + offset, kStart + offset};
}

std::u16string FormatNumber(const scada::DataValue& data_value) {
  double value = 0;
  data_value.value.get(value);
  return UtfConvert<char16_t>(std::to_string(static_cast<int>(value)));
}

std::u16string TimeText(scada::Duration offset) {
  return UtfConvert<char16_t>(
      FormatTime(kStart + offset, TIME_FORMAT_DATE | TIME_FORMAT_TIME |
                                      TIME_FORMAT_MSEC | TIME_FORMAT_UTC));
}

std::vector<std::vector<std::u16string>> ReadAll(GraphExportTable& table) {
  std::vector<std::vector<std::u16string>> rows;
  for (int row = 0; row < table.GetRowCount(); ++row) {
    std::vector<std::u16string> cells;
    for (const auto& column : table.columns())
      cells.push_back(table.GetCellText(row, column.id));
    rows.push_back(std::move(cells));
  }
  return rows;
}

}  // namespace

TEST(GraphExportTableTest, OneRowPerRecordedMomentAndOneColumnPerSeries) {
  const std::vector<scada::DataValue> pressure = {Sample(1, 0min),
                                                  Sample(2, 10min)};
  const std::vector<scada::DataValue> flow = {Sample(7, 5min),
                                              Sample(8, 10min)};
  const std::vector<GraphExportSeries> series = {
      {u"Pressure", pressure, FormatNumber}, {u"Flow", flow, FormatNumber}};

  GraphExportTable table{series, kStart, kStart + 1h, /*utc=*/true};

  ASSERT_EQ(table.columns().size(), 3u);
  EXPECT_EQ(table.columns()[1].title, u"Pressure");
  EXPECT_EQ(table.columns()[2].title, u"Flow");
  EXPECT_EQ(table.columns()[0].data_type,
            scada::aui::TableColumn::DataType::DateTime);

  // A series fills only the moments it recorded; nothing is invented.
  EXPECT_THAT(ReadAll(table),
              ElementsAre(ElementsAre(TimeText(0min), u"1", u""),
                          ElementsAre(TimeText(5min), u"", u"7"),
                          ElementsAre(TimeText(10min), u"2", u"8")));
}

TEST(GraphExportTableTest, TakesOnlyTheVisibleRange) {
  const std::vector<scada::DataValue> values = {
      Sample(1, 0min), Sample(2, 30min), Sample(3, 60min), Sample(4, 90min)};
  const std::vector<GraphExportSeries> series = {
      {u"Level", values, FormatNumber}};

  GraphExportTable table{series, kStart + 30min, kStart + 60min,
                         /*utc=*/true};

  EXPECT_THAT(ReadAll(table), ElementsAre(ElementsAre(TimeText(30min), u"2"),
                                          ElementsAre(TimeText(60min), u"3")));
}

TEST(GraphExportTableTest, AGraphWithNoSeriesExportsTheTimeColumnOnly) {
  GraphExportTable table{{}, kStart, kStart + 1h};
  EXPECT_EQ(table.columns().size(), 1u);
  EXPECT_EQ(table.GetRowCount(), 0);
}
