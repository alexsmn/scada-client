#pragma once

#include "core/node_id.h"

class DialogService;
class Profile;
class TimedDataService;

void ExecuteWriteDialog(DialogService& dialog_service,
                        const scada::NodeId& item_id,
                        bool manual,
                        TimedDataService& timed_data_service,
                        Profile& profile);
