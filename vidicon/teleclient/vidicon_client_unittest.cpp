#include "vidicon/teleclient/vidicon_client.h"

#include "base/test/test_executor.h"
#include "base/win/scoped_bstr.h"
#include "model/namespaces.h"
#include "model/node_id_util.h"
#include "opc/opc_node_ids.h"
#include "scada/attribute_service.h"
#include "timed_data/timed_data_mock.h"
#include "timed_data/timed_data_observer.h"
#include "timed_data/timed_data_property.h"
#include "timed_data/timed_data_service_mock.h"
#include "vidicon/teleclient/test/com_data_point_events.h"
#include "vidicon/teleclient/test/com_event_connector.h"

#include <TeleClient.h>
#include <atlcomcli.h>
#include <gmock/gmock.h>

#include "base/debug_util-inl.h"

using namespace testing;

namespace vidicon {

class VidiconClientTest : public Test {
 protected:
  Microsoft::WRL::ComPtr<IDataPoint> CreateDataPoint(std::wstring_view address);

  const std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();
  NiceMock<MockTimedDataService> timed_data_service_;

  VidiconClient vidicon_client_{
      {.executor_ = executor_, .timed_data_service_ = timed_data_service_}};

  StrictMock<MockFunction<
      void(UINT status, const VARIANT& value, DATE time, UINT quality)>>
      data_change_handler_;
};

namespace {

std::string ToString(std::wstring_view str) {
  return boost::locale::conv::utf_to_utf<char>(str.data(),
                                               str.data() + str.size());
}

}  // namespace

Microsoft::WRL::ComPtr<IDataPoint> VidiconClientTest::CreateDataPoint(
    std::wstring_view address) {
  Microsoft::WRL::ComPtr<IDataPoint> data_point;
  EXPECT_HRESULT_SUCCEEDED(vidicon_client_.teleclient().RequestPoint(
      base::win::ScopedBstr{address}, data_point.ReleaseAndGetAddressOf()));
  EXPECT_THAT(data_point, NotNull());
  return data_point;
}

TEST_F(VidiconClientTest, NewOpcDaDataPoint_ConnectsOpcNode) {
  const std::wstring_view address =
      LR"(VIDICON.Share.1\Стройфарфор.ТС.ВВ-10 ЭГД s3)";

  EXPECT_CALL(timed_data_service_, GetNodeTimedData(opc::MakeOpcNodeId(address),
                                                    /*aggregation*/ _));

  auto data_point = CreateDataPoint(address);

  EXPECT_THAT(timed_data_service_.default_timed_data_->observers_, SizeIs(1));
}

TEST_F(VidiconClientTest, NewOpcAeDataPoint_FailsConnection) {
  const std::wstring_view address =
      LR"(AE:VIDICON.Share.1\Стройфарфор.ТС.ВВ-10 ЭГД s3)";

  Microsoft::WRL::ComPtr<IDataPoint> data_point;
  ASSERT_HRESULT_FAILED(vidicon_client_.teleclient().RequestPoint(
      base::win::ScopedBstr{address}, data_point.ReleaseAndGetAddressOf()));
  ASSERT_EQ(data_point, nullptr);
}

TEST_F(VidiconClientTest, NewVidiconDataPoint_ConnectsVidiconNode) {
  const std::wstring_view address = L"CF:456";

  EXPECT_CALL(timed_data_service_,
              GetNodeTimedData(scada::NodeId{456, NamespaceIndexes::VIDICON},
                               /*aggregation*/ _));

  auto data_point = CreateDataPoint(address);

  EXPECT_THAT(timed_data_service_.default_timed_data_->observers_, SizeIs(1));
}

TEST_F(VidiconClientTest, ReleaseDataPoint_DisconnectsTimedData) {
  auto data_point = CreateDataPoint(L"address");
  ASSERT_THAT(data_point, NotNull());

  // Disconnect.

  data_point.Reset();

  EXPECT_THAT(timed_data_service_.default_timed_data_->observers_, IsEmpty());
}

TEST_F(VidiconClientTest, ReceiveDataPointEvents) {
  auto data_point = CreateDataPoint(L"address");
  ASSERT_THAT(data_point, NotNull());

  // Connect events.

  ComEventConnector event_connector;
  EXPECT_HRESULT_SUCCEEDED(event_connector.Connect(
      *data_point.Get(),
      *CreateComDataPointEvents(
           {.data_changed = data_change_handler_.AsStdFunction()})
           .Get()));

  // Data change.

  ON_CALL(*timed_data_service_.default_timed_data_, GetDataValue())
      .WillByDefault(Return(scada::MakeReadResult(12345)));

  EXPECT_CALL(
      data_change_handler_,
      Call(/*status*/ S_OK, /*value*/
           AllOf(Field(&VARIANT::vt, VT_I4), Field(&VARIANT::intVal, 12345)),
           /*time*/ _, /*quality*/ 192 /*OPC_QUALITY_GOOD*/));

  ASSERT_THAT(timed_data_service_.default_timed_data_->last_observer(),
              NotNull());

  timed_data_service_.default_timed_data_->last_observer()->OnPropertyChanged(
      PropertySet{PROPERTY_CURRENT});
}

}  // namespace vidicon
