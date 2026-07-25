// `vidicon_display_lib.h` wraps the Vidicon TeleClient SDK, which is
// Windows-only (<TeleClient.h>, <Windows.h>), so this suite is too. The rest of
// `client_vidicon_display_native` builds everywhere because no other
// translation unit in it includes that header.
#ifdef _WIN32

#include "vidicon/display/native/vidicon_display_lib.h"

#include <gmock/gmock.h>

using namespace testing;

namespace scada::vidicon {

TEST(DisplayLibrary, Init_NoExceptionThrown) {
  display_library lib;
}

}  // namespace scada::vidicon

#endif  // _WIN32
