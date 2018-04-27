#pragma once

#include "window_definition.h"
#include "services/profile.h"

inline bool IsModus2(const WindowDefinition& definition, Profile& profile) {
  bool modus2 = profile.modus2;
  if (auto* options = definition.FindItem("Options")) {
    auto version = options->GetInt("version", 0);
    if (version != 0)
      modus2 = version >= 2;
  }

  if (!base::LowerCaseEqualsASCII(definition.path.Extension(), ".xsde"))
    modus2 = false;

  return modus2;
}
