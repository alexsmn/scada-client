#pragma once

#include "base/strings/string16.h"
#include "base/win/scoped_bstr.h"
#include "base/win/scoped_comptr.h"
#include "base/win/scoped_variant.h"
#include "common/timed_data/timed_data_spec.h"
#include "client/components/modus/modus.h"

namespace modus {

class ModusObject;

typedef base::win::ScopedComPtr<SDECore::IParam> SDEParam;
typedef base::win::ScopedComPtr<SDECore::IParams> SDEParams;

enum class Limit { LoLo = 0, Lo, Hi, HiHi, Count };

struct Limits {
  double limits[static_cast<size_t>(Limit::Count)];
};

class ModusElement : protected rt::TimedDataDelegate {
 public:
  enum Type { UNKNOWN, SWITCH, LABEL, VALUE };

  enum Style {
    MODUS_INVAL	= 0x0001,	// data item not found
    MODUS_INACT	= 0x0002,	// inactive item (no subsystem)
    MODUS_ALERT	= 0x0004,	// alert state
    MODUS_BADQ	= 0x0008,	// bad quality
  };

  ModusElement(ModusObject& object, SDECore::IParams& sde_params,
               const base::string16& prop_name);

  rt::TimedDataSpec& timed_data() { return data_spec_; }

  unsigned style() const { return style_; }

  void Init();

 protected:
  // rt::TimedDataDelegate
  virtual void OnTimedDataDeleted(rt::TimedDataSpec& spec) override;
  virtual void OnEventsChanged(rt::TimedDataSpec& spec, const events::EventSet& events) override;
  virtual void OnPropertyChanged(rt::TimedDataSpec& spec, const rt::PropertySet& properties) override;
  virtual void OnTimedDataNodeModified(rt::TimedDataSpec& spec, const scada::PropertyIds& property_ids) override;

 private:
  void UpdateData(bool init);
  
  ModusObject& object_;
  SDEParams sde_params_;
  base::string16 prop_name_;

  bool has_limits_ = false;
  Limits limits_{};
  std::vector<base::string16> state_strings_;

  rt::TimedDataSpec	data_spec_;
  
  // data
  base::string16 text_; // for LABEL
  double value_;  // for VALUE

  unsigned style_;
 
  DISALLOW_COPY_AND_ASSIGN(ModusElement);
};

SDEParam GetParam(SDECore::IParams& params, const VARIANT& index);
bool HasParam(SDECore::IParams& params, const VARIANT& index);
base::string16 GetParamValue(SDECore::IParams& params, const VARIANT& index);
bool SetParamValue(SDECore::IParams& params, const VARIANT& index, BSTR val);
base::string16 GetHyperlink(SDECore::ISDEObject50& object);

extern const base::win::ScopedVariant kParameterBinding;
extern const base::win::ScopedVariant kParameterText;
extern const base::win::ScopedVariant kParameterValue;
extern const base::win::ScopedVariant kParameterState;
extern const base::win::ScopedVariant kParameterStyle;

extern const base::win::ScopedBstr kBstrClose;
extern const base::win::ScopedBstr kBstrOpen;

} // namespace modus
