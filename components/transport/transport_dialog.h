#pragma once

namespace net {
class TransportString;
}

class DialogService;

bool ShowTransportDialog(DialogService& dialog_service,
                         net::TransportString& transport_string);
