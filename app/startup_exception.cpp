#include "app/startup_exception.h"

#include "app/client_application.h"

std::optional<std::string> DescribeStartupException(
    std::exception_ptr exception) {
  try {
    std::rethrow_exception(exception);
  } catch (const LoginCanceled&) {
    return std::nullopt;
  } catch (const std::exception& e) {
    return std::string{e.what()};
  } catch (...) {
    return std::string{"unknown exception"};
  }
}

std::optional<std::string> GetStartupErrorMessage(
    std::exception_ptr exception) {
  if (auto description = DescribeStartupException(exception)) {
    return "main() exception: " + *description;
  }
  return std::nullopt;
}
