#include "services/vidicon/vidicon_conversions.h"

#include <gmock/gmock.h>

using namespace testing;

namespace vidicon {

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