#pragma once

#include "base/promise.h"

#include <net/transport_string.h>

class DialogService;

promise<net::TransportString> ShowTransportDialog(
    DialogService& dialog_service,
    const net::TransportString& transport_string);
