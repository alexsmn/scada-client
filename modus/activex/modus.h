#pragma once

#import "modus/activex/typelibs/sdecore.tlb " raw_interfaces_only
#import "modus/activex/typelibs/htsde2.tlb" raw_interfaces_only

using SDECore::ISDEDocument50;

#include <wrl/client.h>

// {E486BCEA-1FB5-42ce-9F89-4938F238D6CE}
// DEFINE_GUID(DIID_Events,
// 0x4677CFA4, 0x78EB, 0x43C0, 0x84, 0xAE, 0xB5, 0xce, 0x30, 0xbf, 0x0f, 0x0d);

// IID DIID_Events = 4677CFA4-78EB-43C0-84AE-B5CE30BF0F0D;

namespace modus {

using ISDEParams = SDECore::IParams52;
using ISDEObject = SDECore::ISDEObject50;
using ISDEObjects = SDECore::ISDEObjects2;

using SDEParam = Microsoft::WRL::ComPtr<SDECore::IParam>;
using SDEParams = Microsoft::WRL::ComPtr<ISDEParams>;
using SDEParamInfo = Microsoft::WRL::ComPtr<SDECore::IParamInfo>;

}  // namespace modus
