#pragma once

#include "base/strings/string16.h"
#include "base/strings/string_piece.h"

namespace scada {
class LocalizedText;
}

base::string16 Translate(base::StringPiece text);

base::string16 ToString16(const scada::LocalizedText& text);
