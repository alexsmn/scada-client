#include "services/vidicon/vidicon_conversions.h"

#include <gmock/gmock.h>

using namespace testing;

namespace vidicon {

TEST(VidiconConversions, ToDATE) {
  EXPECT_EQ(ToDATE(scada::DateTime{}), 0);
  // TODO: Add more specific times.
}

TEST(VidiconConversions, ToOpcQuality) {
  EXPECT_EQ(ToOpcQuality(scada::Qualifier{}), 0xC0 /*OPC_QUALITY_GOOD*/);
  // TODO: Add `ToOpcQuality` tests.
}

TEST(VidiconConversions, ParseOpcDaAddress) {
  EXPECT_EQ(
      ParseDataPointAddress(LR"(VIDICON.Share.1\Стройфарфор.ТС.ВВ-10 ЭГД s3)"),
      DataPointAddress{.opc_address =
                           LR"(VIDICON.Share.1\Стройфарфор.ТС.ВВ-10 ЭГД s3)"});
}

TEST(VidiconConversions, ParseOpcAeAddress) {
  EXPECT_EQ(ParseDataPointAddress(
                LR"(AE:VIDICON.Share.1\Стройфарфор.ТС.ВВ-10 ЭГД s3)"),
            std::nullopt);
}

TEST(VidiconConversions, ParseVidiconAddress) {
  EXPECT_EQ(ParseDataPointAddress(L"CF:456"),
            DataPointAddress{.vidicon_id = 456});
}

}  // namespace vidicon