#pragma once

#include <stdexcept>

// Signals that the user canceled the login dialog rather than a failure.
// Extracted from `client_application.h` so small utilities (notably
// `startup_exception.cpp`) can observe this exception type without pulling
// in the full ClientApplication module graph.
class LoginCanceled : public std::runtime_error {
 public:
  LoginCanceled() : std::runtime_error{"Login canceled"} {}
};
