#pragma once

#include "base/awaitable.h"

#include <transport/transport_string.h>

class DialogService;

Awaitable<transport::TransportString> ShowTransportDialog(
    DialogService& dialog_service,
    const transport::TransportString& transport_string);
