#pragma once

#include "base/win/scoped_comptr.h"
#include "components/modus/activex/sdecore.h"

#include <vector>

namespace SDECore {
struct ISDEObject50;
struct IParams;
}  // namespace SDECore

namespace modus {

class ModusElement;

class ModusObject {
 public:
  typedef std::vector<ModusElement*> Elements;

  explicit ModusObject(SDECore::ISDEObject50& sde_object);
  ~ModusObject();

  SDECore::ISDEObject50& sde_object() const { return *sde_object_; }
  const Elements& elements() const { return elements_; }

  void AddElement(ModusElement& element) { elements_.push_back(&element); }

  void Init();
  void UpdateStyle(bool init);

 private:
  base::win::ScopedComPtr<SDECore::ISDEObject50> sde_object_;

  Elements elements_;

  unsigned current_states_;

  DISALLOW_COPY_AND_ASSIGN(ModusObject);
};

}  // namespace modus
