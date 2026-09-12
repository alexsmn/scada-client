#include "profile/profile_envelope.h"

namespace profile_envelope {
namespace {

// The object at `key`, or nullptr when absent or not an object.
const boost::json::object* FindObject(const boost::json::value& value,
                                      std::string_view key) {
  if (!value.is_object())
    return nullptr;
  const boost::json::value* found = value.as_object().if_contains(key);
  return found && found->is_object() ? &found->as_object() : nullptr;
}

}  // namespace

bool IsEnvelope(const boost::json::value& value) {
  return FindObject(value, "qt") != nullptr ||
         FindObject(value, "web") != nullptr;
}

boost::json::value Unwrap(const boost::json::value& value) {
  if (!IsEnvelope(value))
    return value;

  const boost::json::object* qt = FindObject(value, "qt");
  if (!qt)
    return boost::json::object{};

  const boost::json::value* profile = qt->if_contains("profile");
  if (!profile || !profile->is_object())
    return boost::json::object{};

  return *profile;
}

boost::json::value Wrap(const boost::json::value& flat,
                        const boost::json::value& envelope) {
  boost::json::object next =
      envelope.is_object() ? envelope.as_object() : boost::json::object{};

  next["version"] = 1;

  // Replace `qt.profile` and nothing else under `qt`: a future Qt-owned key
  // beside `profile` is someone else's to keep, the same way `web` is.
  boost::json::object qt;
  if (const boost::json::object* existing = FindObject(envelope, "qt"))
    qt = *existing;
  qt["profile"] =
      flat.is_object() ? flat : boost::json::value{boost::json::object{}};
  next["qt"] = std::move(qt);

  // The web client writes this unconditionally, so an envelope that has been
  // through either client carries it; adding it here keeps a Qt-first write
  // from producing a document the web side then has to special-case.
  if (!next.contains("extras"))
    next["extras"] = boost::json::object{};

  return next;
}

}  // namespace profile_envelope
