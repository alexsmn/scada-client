#pragma once

namespace scada::screenshot_generator {

// On Windows, prints the faulting thread's call stack to stderr when an access
// violation is first raised, before GoogleTest's own SEH handler turns it into
// "SEH exception with code 0xc0000005 thrown in ..." and nothing else. A no-op
// on every other platform, and on repeated calls.
//
// It exists for backlog 950: the generator's first test crashed in its
// fixture's destructor on the Windows CI runner only, and that one line was
// all the run reported. The trace names functions and lines only where the
// build carries debug information, which is why the client's Windows CI leg
// compiles with /Z7 and links with /DEBUG.
void InstallCrashTrace();

}  // namespace scada::screenshot_generator
