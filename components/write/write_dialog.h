#pragma once

#include "core/node_id.h"

class DialogService;
class Profile;
class TimedDataService;

class Profile;
class TimedDataService;

struct WriteContext {
  TimedDataService& timed_data_service_;
  const scada::NodeId node_id_;
  Profile& profile_;
  const bool manual_ = false;
};

void ExecuteWriteDialog(DialogService& dialog_service, WriteContext&& context);
