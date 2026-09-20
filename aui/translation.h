#pragma once

#include <string>
#include <string_view>

std::u16string Translate(std::string_view text);

// The language the UI is currently displayed in, as an RFC 3066 / BCP 47 tag
// ("ru", "en-GB"). This is the user's explicit choice when they made one, and
// the operating system's language otherwise — the same resolution the
// translators are loaded with, so the text the client renders itself and the
// text it asks a server for are always the same language.
//
// Sent as the session's LocaleIds so server-supplied text — node display
// names, event messages — arrives in it (OPC UA Part 4 §5.4 Locale
// Negotiation, https://reference.opcfoundation.org/Core/Part4/v105/docs/5.4).
// Empty when no language can be determined, which the spec reads as "any
// locale the server has".
std::string UiLocaleName();
