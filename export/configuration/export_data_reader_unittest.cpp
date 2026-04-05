#include "export_data_reader.h"

#include "address_space/test/test_scada_node_states.h"
#include "base/csv_reader.h"
#include "node_service/static/static_node_service.h"

#include "base/utf_convert.h"
#include <gmock/gmock.h>
#include <sstream>

using namespace testing;

class ExportDataReaderTest : public Test {
 public:
  virtual void SetUp() override;

 protected:
  ExportData ReadExportData(std::u16string_view str);

  StaticNodeService node_service_;
};

void ExportDataReaderTest::SetUp() {
  node_service_.AddAll(GetScadaNodeStates());
}

TEST_F(ExportDataReaderTest, ReadData) {
  auto data = ReadExportData(
      uR"(��,��������,���,���,����� @SCADA.140,�������� @SCADA.143,�������� @SCADA.144,�������� ���� @SCADA.145,��������� ���� @SCADA.146,����� @SCADA.147,������� ���������� @SCADA.148,"�����������, � @SCADA.149",���������� @SCADA.151,����������� ���������� @SCADA.309,����������� ������ @SCADA.150,����� �������� @SCADA.152,�������� @SCADA.153,��������� @SCADA.154,������ @SCADA.155,�������������� @SCADA.156,����������� ��������� @SCADA.157,���������� ��������� @SCADA.158,���������� �������� @SCADA.159,���������� ��������� @SCADA.160,���������� �������� @SCADA.161,������� ������ ������������� @SCADA.162,������� ������� ������������� @SCADA.163,������� ������ ��������� @SCADA.164,������� ������� ��������� @SCADA.165,������� ��������� @SCADA.166,�������� @SCADA.295,������� ���� @SCADA.296
TS.1,GROUP.1,������ �� @SCADA.72,��,ts101,,3,{IEC_DEV.1!101},,,,0,,,,�� (60 ����) @HISTORICAL_DB.1,0,��������/�������� @TS_PARAMS.3,,,,,,,,,,,,,,
TIT.1,GROUP.1,������ ��� @SCADA.76,Ia,tit1136,,1,{IEC_DEV.1!1136},,,,0,,,,��� (30 ����) @HISTORICAL_DB.2,,,0.#,0,0,0,2000,0,2000,120,180,100,200,,0,0
TS.22,GROUP.1,������ �� @SCADA.72,��������� ������������,ts106,0,90,{IEC_DEV.1!106},,,,0,0,1,,�� (60 ����) @HISTORICAL_DB.1,0,���/�� @TS_PARAMS.2,,,,,,,,,,,,,,
TS.23,GROUP.1,������ �� @SCADA.72,�����������. ������������,ts105,0,70,{IEC_DEV.1!105},,,,0,0,1,,�� (60 ����) @HISTORICAL_DB.1,0,���/�� @TS_PARAMS.2,,,,,,,,,,,,,,
TS.24,GROUP.1,������ �� @SCADA.72,"������.�� - ""�����.""",ts107,0,3,{IEC_DEV.1!107},,,,0,0,1,,�� (60 ����) @HISTORICAL_DB.1,0,���/�� @TS_PARAMS.2,,,,,,,,,,,,,,
TIT.75,GROUP.1,������ ��� @SCADA.76,P,tit1142,0,1,{IEC_DEV.1!1142},,,,0,0,1,,��� (30 ����) @HISTORICAL_DB.2,,,0.#,0,0,0,20000,0,20000,,,,,,0,0
TIT.76,GROUP.1,������ ��� @SCADA.76,Q,tit1143,0,1,{IEC_DEV.1!1143},,,,0,0,1,,��� (30 ����) @HISTORICAL_DB.2,,,0.#,0,0,0,20000,0,20000,,,,,,0,0)");

  EXPECT_EQ(data.nodes.size(), 7);
}

ExportData ExportDataReaderTest::ReadExportData(std::u16string_view str) {
  auto raw_str = UtfConvert<char>(std::u16string{str});
  std::istringstream stream{raw_str};
  CsvReader csv_reader{stream};
  ExportDataReader reader{node_service_, csv_reader};
  return reader.Read();
}