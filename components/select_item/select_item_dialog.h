#pragma once

#include "node_id_set.h"

class DialogService;
class NodeService;

NodeIdSet RunSelectItemsDialog(DialogService& dialog_service,
                               NodeService& node_service);
