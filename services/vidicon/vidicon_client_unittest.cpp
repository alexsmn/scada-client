#include "services/vidicon/vidicon_client.h"

#include "base/string_piece_util.h"
#include "base/test/test_executor.h"
#include "base/win/scoped_bstr.h"
#include "services/vidicon/teleclient.h"
#include "services/vidicon/test/com_data_point_events.h"
#include "services/vidicon/test/com_event_connector.h"
#include "timed_data/timed_data_delegate.h"
#include "timed_data/timed_data_mock.h"
#include "timed_data/timed_data_service_mock.h"

#include <atlcomcli.h>
#include <gmock/gmock.h>

using namespace testing;

namespace vidicon {

class VidiconClientTest : public Test {
 protected:
  const std::shared_ptr<Executor> executor_ = std::make_shared<TestExecutor>();
  NiceMock<MockTimedDataService> timed_data_service_;

  VidiconClient vidicon_client_{
      {.executor_ = executor_, .timed_data_service_ = timed_data_service_}};
};

TEST_F(VidiconClientTest, Test) {
  const std::wstring_view address = L"address";

  Microsoft::WRL::ComPtr<IDataPoint> data_point;
  ASSERT_HRESULT_SUCCEEDED(vidicon_client_.teleclient().RequestPoint(
      base::win::ScopedBstr{AsStringPiece(address)},
      data_point.ReleaseAndGetAddressOf()));
  ASSERT_THAT(data_point, NotNull());

  EXPECT_THAT(timed_data_service_.default_timed_data_->observers_, SizeIs(1));

  // Subscribe

  {
    MockFunction<void(const VARIANT& value, DATE time, UINT quality)>
        data_change_handler;

    ComEventConnector event_connector;
    EXPECT_HRESULT_SUCCEEDED(event_connector.Connect(
        *data_point.Get(),
        *CreateComDataPointEvents(
             {.data_changed = data_change_handler.AsStdFunction()})
             .Get()));

    // Data change.

    auto data_value = scada::MakeReadResult(12345);

    ON_CALL(*timed_data_service_.default_timed_data_, GetDataValue())
        .WillByDefault(Return(data_value));

    EXPECT_CALL(data_change_handler,
                Call(/*value*/ AllOf(Field(&VARIANT::vt, VT_I4),
                                     Field(&VARIANT::intVal, 12345)),
                     /*time*/ _, /*quality*/ 192 /*OPC_QUALITY_GOOD*/));

    ASSERT_THAT(timed_data_service_.default_timed_data_->last_observer(),
                NotNull());

    timed_data_service_.default_timed_data_->last_observer()->OnPropertyChanged(
        PropertySet{PROPERTY_CURRENT});
  }

  // Disconnect.

  data_point.Reset();

  EXPECT_THAT(timed_data_service_.default_timed_data_->observers_, IsEmpty());
}

}  // namespace vidicon
