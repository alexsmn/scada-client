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

inline std::u16string GetResourceErrorMessage(std::exception_ptr e) {
  try {
    std::rethrow_exception(e);
  } catch (const ResourceError& e) {
    return e.message();
  } catch (...) {
    return u"Ошибка при экспорте.";
  }
}
