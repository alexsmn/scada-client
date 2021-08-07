#pragma once

#include <optional>

namespace base {
class Value;
}

template <class T>
std::optional<T> FromJson(const base::Value& value);
