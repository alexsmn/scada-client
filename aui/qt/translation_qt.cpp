#include "aui/translation.h"

#include <QObject>

std::u16string Translate(std::string_view text) {
  return QObject::tr(std::string(text).c_str()).toStdU16String();
}
