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
      uR"(Ид,Родитель,Тип,Имя,Алиас @SCADA.140,Эмуляция @SCADA.143,Важность @SCADA.144,Основной ввод @SCADA.145,Резервный ввод @SCADA.146,Вывод @SCADA.147,Условие управления @SCADA.148,"Устаревание, с @SCADA.149",Блокировка @SCADA.151,Двухэтапное управление @SCADA.309,Эмулируемый сигнал @SCADA.150,Архив значений @SCADA.152,Инверсия @SCADA.153,Параметры @SCADA.154,Формат @SCADA.155,Преобразование @SCADA.156,Ограничение диапазона @SCADA.157,Логический минимимум @SCADA.158,Логический максимум @SCADA.159,Физический минимимум @SCADA.160,Физический максимум @SCADA.161,Уставка нижняя предаварийная @SCADA.162,Уставка верхняя предаварийная @SCADA.163,Уставка нижняя аварийная @SCADA.164,Уставка верхняя аварийная @SCADA.165,Единицы измерения @SCADA.166,Апертура @SCADA.295,Мертвая зона @SCADA.296
TS.1,GROUP.1,Объект ТС @SCADA.72,ЗР,ts101,,3,{IEC_DEV.1!101},,,,0,,,,ТС (60 дней) @HISTORICAL_DB.1,0,Отключен/Заземлен @TS_PARAMS.3,,,,,,,,,,,,,,
TIT.1,GROUP.1,Объект ТИТ @SCADA.76,Ia,tit1136,,1,{IEC_DEV.1!1136},,,,0,,,,ТИТ (30 дней) @HISTORICAL_DB.2,,,0.#,0,0,0,2000,0,2000,120,180,100,200,,0,0
TS.22,GROUP.1,Объект ТС @SCADA.72,Аварийная сигнализация,ts106,0,90,{IEC_DEV.1!106},,,,0,0,1,,ТС (60 дней) @HISTORICAL_DB.1,0,Нет/Да @TS_PARAMS.2,,,,,,,,,,,,,,
TS.23,GROUP.1,Объект ТС @SCADA.72,Предупредит. сигнализация,ts105,0,70,{IEC_DEV.1!105},,,,0,0,1,,ТС (60 дней) @HISTORICAL_DB.1,0,Нет/Да @TS_PARAMS.2,,,,,,,,,,,,,,
TS.24,GROUP.1,Объект ТС @SCADA.72,"Блокир.ТУ - ""Местн.""",ts107,0,3,{IEC_DEV.1!107},,,,0,0,1,,ТС (60 дней) @HISTORICAL_DB.1,0,Нет/Да @TS_PARAMS.2,,,,,,,,,,,,,,
TIT.75,GROUP.1,Объект ТИТ @SCADA.76,P,tit1142,0,1,{IEC_DEV.1!1142},,,,0,0,1,,ТИТ (30 дней) @HISTORICAL_DB.2,,,0.#,0,0,0,20000,0,20000,,,,,,0,0
TIT.76,GROUP.1,Объект ТИТ @SCADA.76,Q,tit1143,0,1,{IEC_DEV.1!1143},,,,0,0,1,,ТИТ (30 дней) @HISTORICAL_DB.2,,,0.#,0,0,0,20000,0,20000,,,,,,0,0)");

  EXPECT_EQ(data.nodes.size(), 7);
}

ExportData ExportDataReaderTest::ReadExportData(std::u16string_view str) {
  auto raw_str = UtfConvert<char>(std::u16string{str});
  std::istringstream stream{raw_str};
  CsvReader csv_reader{stream};
  ExportDataReader reader{node_service_, csv_reader};
  return reader.Read();
}