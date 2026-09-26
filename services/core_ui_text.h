#pragma once

#include "common/format.h"
#include "scada/status.h"

#include <string>

// The client's words for values core and common describe only by invariant
// forms — status codes, quality flags, booleans and common's fallback labels —
// in the display locale.
//
// These tables are the only place those words live: core and common carry
// invariant forms alone and ask for text through the providers
// `InstallCoreUiText` installs, so every `ToString16(status)`,
// `ToString16(qualifier)`, `Variant::TrueLabel()` and `DefaultCloseLabel()` in
// shared code renders through them.

// The description of `status_code`. Codes the table does not know fall back to
// a generic success or error sentence by severity.
std::u16string StatusText(scada::StatusCode status_code);

// The word for one quality flag bit (`scada::Qualifier::STALE`), or an empty
// string for a bit the table does not know.
std::u16string QualifierFlagText(unsigned flag);

// The word for a boolean value.
std::u16string BooleanText(bool value);

// The text of one of common's fallback labels (`common/format.h`).
std::u16string FallbackLabelText(FallbackLabel label);

// Installs all four as core's and common's providers. Call once at startup,
// before any such text is rendered; `AppInit` and the screenshot fixture both
// do.
void InstallCoreUiText();
