#pragma once

#include <stdexcept>
#include <string>

class ResourceError {
 public:
  explicit ResourceError(std::wstring message) : message_{std::move(message)} {}

  const std::wstring& message() const { return message_; }

 private:
  const std::wstring message_;
};
