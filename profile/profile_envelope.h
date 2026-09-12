#pragma once

#include <boost/json.hpp>

// The profile envelope: the document layout both clients use on the server.
//
// The Qt client's own profile document is FLAT -- `pages`, `favorites`,
// `showWriteOk` and the rest at the top level -- and that is what
// `Profile::SaveToValue` produces and `Profile::Load` reads. The document kept
// on the server is not that shape. It is an envelope with one section per
// client:
//
//     {"version": 1,
//      "qt":  {"profile": { <the flat Qt document> }},
//      "web": { <the web client's own settings> },
//      "extras": {}}
//
// so that each client can keep its own settings beside the other's, and share
// the ones that are genuinely one thing -- pages and favourites live in
// `qt.profile` and are read and written by both.
//
// **Preserving the section you do not own is the whole point.** A client that
// writes its own document over the variable destroys the other's settings, and
// nothing reports it: the next read simply comes back without them. `Wrap`
// therefore takes the envelope that was last read and returns it with only the
// Qt section replaced, which mirrors what the web client's
// `encodeProfileEnvelope` does for the web section.
//
// See `web/packages/ua-react/src/stores/profile-store.ts` for the other half of
// this contract, and `web/docs/architecture.md` for why it is shaped this way.
namespace profile_envelope {

// True when `value` is an envelope rather than a flat Qt profile document.
//
// Decided on the sections being present as objects, not on `version`: a flat
// Qt document has no `qt` or `web` key of its own (nothing under `client/`
// writes one), while an envelope always has at least the section its writer
// owns. A fresh profile written by the web client carries an empty
// `qt.profile`, so it is still recognised.
bool IsEnvelope(const boost::json::value& value);

// The flat Qt profile document inside `value`.
//
// Returns `value` itself when it is already flat, so a caller can hand this
// whatever it read without asking first -- a file written by an older build, a
// server document written by either client.
//
// An envelope with no Qt section yields an empty object rather than the
// envelope: handing `Profile::Load` an envelope makes every key it looks for
// miss, which loads silently as an empty profile and then overwrites the real
// one on exit. Empty is the same outcome minus the pretence.
boost::json::value Unwrap(const boost::json::value& value);

// `flat` wrapped into `envelope`, which is the envelope previously read.
//
// Every section `envelope` carries survives except `qt.profile`, which is
// replaced. Pass a null or non-object `envelope` for "there was none" -- the
// result is then a fresh envelope carrying only the Qt section.
boost::json::value Wrap(const boost::json::value& flat,
                        const boost::json::value& envelope);

}  // namespace profile_envelope
