#include "components/transport/transport_dialog.h"

#include "components/transport/transport_dialog_model.h"
#include "net/transport_string.h"
#include "aui/dialog_service.h"

promise<net::TransportString> ShowTransportDialog(
    DialogService& dialog_service,
    const net::TransportString& transport_string) {
  return make_rejected_promise<net::TransportString>(std::exception{});
}
