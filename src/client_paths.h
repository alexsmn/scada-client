#pragma once

namespace client {

enum {
  PATH_START = 10000,

  DIR_INSTALL,

  DIR_DATA,

  DIR_PUBLIC,

  DIR_PRIVATE,

  DIR_DOCUMENTATION,

  DIR_LOG,

  PATH_END
};

void RegisterPathProvider();

} // namespace client
