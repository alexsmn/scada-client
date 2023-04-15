#pragma once

#include "base/win/scoped_bstr.h"
#include "base/win/scoped_variant.h"
#include "components/modus/activex/modus.h"
#include "timed_data/timed_data_spec.h"

#include <string>

namespace modus {

class ModusObject;

enum class Limit { LoLo = 0, Lo, Hi, HiHi, Count };

struct Limits {
  double limits[static_cast<size_t>(Limit::Count)];
};

struct ModusElementContext {
  ModusObject& object_;
  const SDEParams sde_params_;
  const std::wstring prop_name_;
  const std::vector<std::wstring> state_strings_;
  const bool has_limits_ = false;
};

class ModusElement : private ModusElementContext {
 public:
  enum Type { UNKNOWN, SWITCH, LABEL, VALUE };

  enum Style {
    MODUS_INVAL = 0x0001,  // data item not found
    MODUS_INACT = 0x0002,  // inactive item (no subsystem)
    MODUS_ALERT = 0x0004,  // alert state
    MODUS_BADQ = 0x0008,   // bad quality
  };

  explicit ModusElement(ModusElementContext&& context);

  ModusElement(ModusElement&&) = default;
  ModusElement& operator=(ModusElement&&) = default;

  TimedDataSpec& timed_data() { return data_spec_; }

  unsigned style() const { return style_; }

  void Init();

 private:
  void UpdateData(bool init);

  Limits limits_{};

  TimedDataSpec data_spec_;

  // data
  std::wstring text_;  // for LABEL
  double value_ = 0;   // for VALUE

  unsigned style_ = 0;
};

SDEParam GetParam(ISDEParams& params, const VARIANT& index);
bool HasParam(ISDEParams& params, const VARIANT& index);
std::wstring GetParamValue(ISDEParams& params, const VARIANT& index);
bool SetParamValue(ISDEParams& params, const VARIANT& index, BSTR value);
bool SetParamValue(ISDEParams& params, std::wstring_view index, BSTR value);
std::wstring GetHyperlink(ISDEObject& object);

extern const base::win::ScopedVariant kParameterBinding;
extern const base::win::ScopedVariant kParameterText;
extern const base::win::ScopedVariant kParameterValue;
extern const base::win::ScopedVariant kParameterState;
extern const base::win::ScopedVariant kParameterStyle;
extern const base::win::ScopedVariant kParameterLimits;

extern const base::win::ScopedBstr kBstrClose;
extern const base::win::ScopedBstr kBstrOpen;

}  // namespace modus
