#include "client_utils.h"

#include "base/strings/sys_string_conversions.h"
#include "translation.h"

std::wstring Translate(const char* text) {
  return base::SysNativeMBToWide(text);
}

std::wstring FormatHostName(const std::string& host_name) {
  return base::SysNativeMBToWide(host_name);
}
