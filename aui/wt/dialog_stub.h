#pragma once

#include "base/promise.h"

namespace aui::wt {

template <class T>
promise<T> MakeUnsupportedDialogPromise() {
  return MakeRejectedPromise<T>();
}

}  // namespace aui::wt
