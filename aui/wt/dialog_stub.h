#pragma once

#include "aui/aui_ns_compat.h"

#include "base/awaitable.h"

#include <exception>

namespace scada::aui::wt {

template <class T>
Awaitable<T> MakeUnsupportedDialogAwaitable() {
  throw std::exception{};
}

}  // namespace aui::wt
