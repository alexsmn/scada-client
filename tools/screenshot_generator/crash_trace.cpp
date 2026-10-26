#include "crash_trace.h"

#if defined(_WIN32)

#include <windows.h>

// DbgHelp after windows.h, which it requires.
#include <dbghelp.h>

#include <atomic>
#include <cstdio>

namespace scada::screenshot_generator {
namespace {

// Enough to see a teardown fault without flooding the log if something faults
// repeatedly and is handled each time.
constexpr int kMaxTraces = 3;
constexpr int kMaxFrames = 64;

// Prints one frame as "#n module!function+offset (file:line)", with whatever
// of that the symbols allow.
void PrintFrame(HANDLE process, int index, DWORD64 address) {
  char module_name[MAX_PATH] = "?";
  if (const DWORD64 module_base = SymGetModuleBase64(process, address)) {
    GetModuleFileNameA(reinterpret_cast<HMODULE>(module_base), module_name,
                       MAX_PATH);
  }

  alignas(SYMBOL_INFO) char symbol_buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME];
  auto* symbol = reinterpret_cast<SYMBOL_INFO*>(symbol_buffer);
  symbol->SizeOfStruct = sizeof(SYMBOL_INFO);
  symbol->MaxNameLen = MAX_SYM_NAME;
  DWORD64 displacement = 0;
  const bool have_symbol =
      SymFromAddr(process, address, &displacement, symbol) != FALSE;

  IMAGEHLP_LINE64 line{};
  line.SizeOfStruct = sizeof(line);
  DWORD line_displacement = 0;
  const bool have_line =
      SymGetLineFromAddr64(process, address, &line_displacement, &line) !=
      FALSE;

  std::fprintf(stderr, "  #%d %s!%s+0x%llx", index, module_name,
               have_symbol ? symbol->Name : "?",
               static_cast<unsigned long long>(displacement));
  if (have_line)
    std::fprintf(stderr, " (%s:%lu)", line.FileName, line.LineNumber);
  std::fprintf(stderr, "\n");
}

// Walks the stack from the context the fault was raised in, which is the
// faulting frame itself rather than this handler's.
LONG CALLBACK TraceAccessViolation(EXCEPTION_POINTERS* info) {
  if (info->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
    return EXCEPTION_CONTINUE_SEARCH;

  static std::atomic<int> traces{0};
  if (traces.fetch_add(1) >= kMaxTraces)
    return EXCEPTION_CONTINUE_SEARCH;

  const HANDLE process = GetCurrentProcess();
  const HANDLE thread = GetCurrentThread();
  SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_DEFERRED_LOADS | SYMOPT_UNDNAME);
  SymInitialize(process, nullptr, TRUE);

  const auto* record = info->ExceptionRecord;
  std::fprintf(
      stderr,
      "crash_trace: access violation (%s 0x%llx) at 0x%llx, thread "
      "%lu\n",
      record->NumberParameters >= 2 && record->ExceptionInformation[0]
          ? "writing"
          : "reading",
      record->NumberParameters >= 2
          ? static_cast<unsigned long long>(record->ExceptionInformation[1])
          : 0ULL,
      reinterpret_cast<unsigned long long>(record->ExceptionAddress),
      GetCurrentThreadId());

  CONTEXT context = *info->ContextRecord;
  STACKFRAME64 frame{};
  frame.AddrPC.Offset = context.Rip;
  frame.AddrPC.Mode = AddrModeFlat;
  frame.AddrFrame.Offset = context.Rbp;
  frame.AddrFrame.Mode = AddrModeFlat;
  frame.AddrStack.Offset = context.Rsp;
  frame.AddrStack.Mode = AddrModeFlat;

  for (int index = 0; index < kMaxFrames; ++index) {
    if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &frame,
                     &context, nullptr, SymFunctionTableAccess64,
                     SymGetModuleBase64, nullptr) ||
        frame.AddrPC.Offset == 0) {
      break;
    }
    PrintFrame(process, index, frame.AddrPC.Offset);
  }
  std::fflush(stderr);

  // Let the fault take its normal course: GoogleTest still reports it.
  return EXCEPTION_CONTINUE_SEARCH;
}

}  // namespace

void InstallCrashTrace() {
  static std::atomic<bool> installed{false};
  if (installed.exchange(true))
    return;
  // First in the chain, so it sees the fault before any frame-based handler.
  AddVectoredExceptionHandler(/*First=*/1, TraceAccessViolation);
}

}  // namespace scada::screenshot_generator

#else

namespace scada::screenshot_generator {

void InstallCrashTrace() {}

}  // namespace scada::screenshot_generator

#endif
