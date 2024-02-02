#pragma once

#include <stdexcept>
#include <string>

class ResourceError {
 public:
  explicit ResourceError(std::u16string message)
      : message_{std::move(message)} {}

  const std::u16string& message() const { return message_; }

 private:
  const std::u16string message_;
};
