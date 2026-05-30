#pragma once

#ifdef _WIN32
#include <atlbase.h>

class DummyAtlModule : public CAtlExeModuleT<DummyAtlModule> {};
#else
class DummyAtlModule {};
#endif

extern DummyAtlModule _Module;
