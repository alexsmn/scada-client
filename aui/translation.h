#pragma once

#include <string>
#include <string_view>

std::u16string Translate(std::string_view text);

// The language the UI is currently displayed in, as an RFC 3066 / BCP 47 tag
// ("ru", "en-GB"): the user's explicit choice when they made one, then the
// app's startup override, then the operating system's language.
//
// This is THE resolution — `InstalledTranslation` picks the `.qm` catalogs to
// install with the same call, so the text the client renders itself and the
// text it asks a server for cannot end up in different languages. Two copies
// of this order is exactly the defect: the command-line `--locale` switch used
// to reach the translators only, so `--locale=en` on a Russian machine gave an
// English window full of Russian node names.
//
// Sent as the session's LocaleIds so server-supplied text — node display
// names, event messages — arrives in it (OPC UA Part 4 §5.4 Locale
// Negotiation, https://reference.opcfoundation.org/Core/Part4/v105/docs/5.4).
// Empty when no language can be determined, which the spec reads as "any
// locale the server has".
std::string UiLocaleName();

// Supplies the startup language override — the `--locale` command-line switch
// — which `UiLocaleName()` consults when the operator has stored no explicit
// choice.
//
// It is injected rather than read here because command-line parsing lives in
// the client's own `base/program_options.h`, and `aui/` must not include
// client-repo headers (it is extracted separately; see
// docs/client/aui-extraction.md). Call it once, before the translators are
// installed. An empty string clears the override, which is also the default:
// a client that never calls this behaves exactly as before.
void SetUiLocaleOverride(std::string locale_name);
