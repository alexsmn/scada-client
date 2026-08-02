#pragma once

class DialogService;
struct WriteContext;

class WriteService {
 public:
  virtual ~WriteService() = default;

  virtual void ExecuteWriteDialog(DialogService& dialog_service,
                                  const scada::NodeId& node_id,
                                  bool manual) = 0;
};