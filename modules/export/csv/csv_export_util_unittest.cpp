#include "export/csv/csv_export_util.h"

#include "aui/models/table_model.h"
#include "base/utf_convert.h"
#include "common/test/scoped_temp_dir.h"

#include <fstream>
#include <gmock/gmock.h>

using namespace testing;

namespace {

const std::string kUtf8Bom = "\xEF\xBB\xBF";

// Two rows of Cyrillic, which is what makes the encoding matter.
class CyrillicTableModel : public scada::aui::TableModel {
 public:
  int GetRowCount() override { return 2; }

  void GetCell(scada::aui::TableCell& cell) override {
    cell.text = cell.row == 0 ? u"Да" : u"Нет";
  }
};

// Windows-1251 for ASCII and А..я (U+0410..U+044F → 0xC0..0xFF), standing in
// for the ANSI code page of a Russian Windows on a host that has none.
std::string EncodeFakeCp1251(std::u16string_view utf16) {
  std::string result;
  for (char16_t ch : utf16) {
    if (ch < 0x80)
      result.push_back(static_cast<char>(ch));
    else if (ch >= 0x0410 && ch <= 0x044F)
      result.push_back(static_cast<char>(0xC0 + (ch - 0x0410)));
    else
      result.push_back('?');
  }
  return result;
}

class CsvExportUtilTest : public Test {
 protected:
  // Exports the table with `params` and returns the file's bytes.
  std::string Export(const CsvExportParams& params) {
    ExportModel::TableExportData table{.model = model_, .columns = columns_};
    ExportToCsv(table, params, path_, {.encode = &EncodeFakeCp1251});

    std::ifstream stream{path_, std::ios::binary};
    return {std::istreambuf_iterator<char>{stream},
            std::istreambuf_iterator<char>{}};
  }

  // First member: the exported file lives under it.
  scada::ScopedTempDir temp_dir_{"scada_csv_export_util_test"};
  const std::filesystem::path path_ = temp_dir_.path() / "export.csv";

  CyrillicTableModel model_;
  std::vector<scada::aui::TableColumn> columns_{{.id = 1, .title = u"Имя"}};
};

}  // namespace

TEST_F(CsvExportUtilTest, Utf8WritesBomAndCrLfRows) {
  EXPECT_EQ(Export({.unicode = true}),
            kUtf8Bom +
                "\xD0\x98\xD0\xBC\xD1\x8F\r\n"  // Имя
                "\xD0\x94\xD0\xB0\r\n"          // Да
                "\xD0\x9D\xD0\xB5\xD1\x82");    // Нет
}

TEST_F(CsvExportUtilTest, SystemEncodingConvertsToTheCodePageWithoutBom) {
  EXPECT_EQ(Export({.unicode = false}),
            "\xC8\xEC\xFF\r\n"  // Имя
            "\xC4\xE0\r\n"      // Да
            "\xCD\xE5\xF2");    // Нет
}

// Backlog 947: Excel splits a CSV file on the system list separator, so the
// fixed `,` default produced one-column files under Russian settings.
TEST(CsvDelimiterTest, FollowsTheSystemListSeparator) {
  EXPECT_EQ(ChooseCsvDelimiter(u";", u","), ';');
  EXPECT_EQ(ChooseCsvDelimiter(u",", u"."), ',');
  EXPECT_EQ(ChooseCsvDelimiter(u"|", u"."), '|');
  EXPECT_EQ(ChooseCsvDelimiter(u"\t", u"."), '\t');
}

TEST(CsvDelimiterTest, WithoutAListSeparatorFollowsTheDecimalSeparator) {
  // A decimal comma means the list separator is `;` (Russian, German...).
  EXPECT_EQ(ChooseCsvDelimiter(u"", u","), ';');
  EXPECT_EQ(ChooseCsvDelimiter(u"", u"."), ',');
}

TEST(CsvDelimiterTest, AnUnusableListSeparatorIsIgnored) {
  for (std::u16string_view separator :
       {u"\"", u"0", u"a", u"Z", u" ", u";;", u"‖"}) {
    EXPECT_EQ(ChooseCsvDelimiter(separator, u","), ';')
        << UtfConvert<char>(std::u16string{separator});
  }
}

TEST(CsvDelimiterTest, ASavedDelimiterWinsOverTheDefault) {
  boost::json::value stored{boost::json::object{}};
  EXPECT_EQ(LoadCsvExportParams(stored, ';').delimiter, ';');
  EXPECT_EQ(LoadCsvExportParams(boost::json::value{}, ';').delimiter, ';');

  CsvExportParams saved;
  saved.delimiter = '\t';
  saved.unicode = true;
  const CsvExportParams loaded = LoadCsvExportParams(ToJson(saved), ';');
  EXPECT_EQ(loaded.delimiter, '\t');
  EXPECT_TRUE(loaded.unicode);
}
