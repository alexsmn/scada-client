#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <utility>

// A staged-edit buffer for the reshell device-parameter form: the pending field
// edits the operator has made but not yet written to the model. The form stages
// an edit only when the new text differs from the model's live value, and drops
// it when the text returns to live — so a non-empty buffer means "dirty", and
// the Revert/Apply affordances key off dirty(). Pure (no Qt), so it is
// unit-testable without a QApplication.
class ParameterStaging {
 public:
  // Identifies an edited field: the (property group, index) the value writes to.
  // The group is opaque here — the form owns the typed pointer and replays the
  // write onto it on Apply.
  using Key = std::pair<const void*, int>;

  // Stages `value` for `key`, replacing any prior staged value.
  void Set(const Key& key, std::u16string value);

  // Drops the staged value for `key`, if any (the field returned to live).
  void Remove(const Key& key);

  // The staged value for `key`, or nullptr when the field is unedited.
  const std::u16string* Get(const Key& key) const;

  // Discards every staged edit (Revert, or after a successful Apply).
  void Clear();

  bool dirty() const { return !edits_.empty(); }
  std::size_t size() const { return edits_.size(); }

  // The staged edits, for Apply to replay onto the model.
  const std::map<Key, std::u16string>& edits() const { return edits_; }

 private:
  std::map<Key, std::u16string> edits_;
};
