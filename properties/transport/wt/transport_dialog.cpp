#include "properties/transport/transport_dialog.h"

#include "properties/transport/transport_dialog_model.h"
#include "transport/transport_string.h"
#include "aui/dialog_service.h"

promise<transport::TransportString> ShowTransportDialog(
    DialogService& dialog_service,
    const transport::TransportString& transport_string) {
  return make_rejected_promise<transport::TransportString>(std::exception{});
}
