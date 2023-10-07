#include "components\vidicon_display\qt\vidicon_display_lib.h"

#include <gmock/gmock.h>

using namespace testing;

namespace vidicon {

TEST(DisplayLibrary, Init_NoExceptionThrown) {
  display_library lib;
}

}  // namespace vidicon