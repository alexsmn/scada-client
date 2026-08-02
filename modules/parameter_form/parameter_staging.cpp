#include "parameter_form/parameter_staging.h"

#include <utility>

void ParameterStaging::Set(const Key& key, std::u16string value) {
  edits_[key] = std::move(value);
}

void ParameterStaging::Remove(const Key& key) {
  edits_.erase(key);
}

const std::u16string* ParameterStaging::Get(const Key& key) const {
  auto it = edits_.find(key);
  return it == edits_.end() ? nullptr : &it->second;
}

void ParameterStaging::Clear() {
  edits_.clear();
}
