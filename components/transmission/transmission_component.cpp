#include "components/transmission/transmission_view.h"
#include "controller_factory.h"

const WindowInfo kWindowInfo = {ID_TRANSMISSION_VIEW,
                                "Transmission",
                                L"Ретрансляция",
                                WIN_INS | WIN_DISALLOW_NEW,
                                0,
                                0,
                                0};

REGISTER_CONTROLLER(TransmissionView, kWindowInfo);
