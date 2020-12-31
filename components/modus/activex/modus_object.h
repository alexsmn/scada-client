#pragma once

#include "components/modus/activex/modus.h"

#include <memory>
#include <vector>
#include <wrl/client.h>

namespace SDECore {
struct ISDEObject50;
struct IParams;
}  // namespace SDECore

namespace modus {

class ModusElement;

class ModusObject {
 public:
  typedef std::vector<std::unique_ptr<ModusElement>> Elements;

  explicit ModusObject(SDECore::ISDEObject50& sde_object);
  ~ModusObject();

  ModusObject(const ModusObject&) = delete;
  ModusObject& operator=(const ModusObject&) = delete;

  SDECore::ISDEObject50& sde_object() const { return *sde_object_.Get(); }
  const Elements& elements() const { return elements_; }

  void AddElement(ModusElement& element) { elements_.emplace_back(&element); }

  void Init();
  void UpdateStyle(bool init);

 private:
  Microsoft::WRL::ComPtr<SDECore::ISDEObject50> sde_object_;

  Elements elements_;

  unsigned current_states_;
};

}  // namespace modus
