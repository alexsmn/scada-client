#pragma once

#include "base/promise.h"

#include <transport/transport_string.h>

class DialogService;

promise<transport::TransportString> ShowTransportDialog(
    DialogService& dialog_service,
    const transport::TransportString& transport_string);
