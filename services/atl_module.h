#pragma once

#include <atlbase.h>

class DummyAtlModule : public CAtlExeModuleT<DummyAtlModule> {};

extern DummyAtlModule _Module;
