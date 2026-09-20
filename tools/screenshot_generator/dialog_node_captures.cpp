// Dialog captures that browse the address space rather than operating on one
// item: change password, bulk TS/TI creation, and service-object creation.
//
// What they share is that their lists come from whole subtrees, so they need
// several waves of fetching before the model is built — and the failure when a
// wave is missing is an empty combo or list, which still writes a well-formed
// PNG that `check_screenshots.py` accepts.

#include "dialog_kinds.h"

#include "null_task_manager.h"
#include "screenshot_config.h"

#include "aui/qt/dialog_service_impl_qt.h"
#include "model/data_items_node_ids.h"
#include "model/devices_node_ids.h"
#include "model/node_id_util.h"
#include "modules/change_password/change_password_dialog.h"
#include "modules/create_service_item/create_service_item_dialog.h"
#include "modules/events/local_events.h"
#include "modules/multi_create/multi_create_dialog.h"
#include "node_service/node_ref.h"
#include "node_service/node_service.h"
#include "profile/profile.h"
#include "ui/common/client_utils.h"

#include <QApplication>
#include <gtest/gtest.h>

#include <algorithm>
#include <vector>

namespace scada::screenshot_generator {
namespace {

// Node ids of every DeviceType instance reachable under the Devices folder.
std::vector<scada::NodeId> EnumerateDeviceIds(NodeService& node_service) {
  std::vector<scada::NodeId> ids;
  for (const auto& [name, device] :
       GetNamedNodes(node_service.GetNode(scada::devices::id::Devices),
                     scada::devices::id::DeviceType)) {
    ids.push_back(device.node_id());
  }
  return ids;
}

// Makes the Devices folder, its devices, and the devices nested under those
// devices resident — each wave enumerable only once the previous one landed.
//
// Wave 1 (the folder) yields the device instances but not their components, and
// CreateServiceItemModel reads its component list off the device, one level
// further down, in its constructor. Wave 2 fetches those.
//
// Wave 3 exists because devices nest. GetNamedNodes recurses through anything
// that is itself a DeviceType instance — and a LINK is one (devices.xml derives
// LinkType from DeviceType) — so an RTU parented under its link is a level
// below what wave 2 could even enumerate: the link's children were not resident
// when that list was built. Re-enumerating now finds them. Without this the
// combo silently drops every device that hangs under a link, which is how a
// real address space is shaped.
bool FetchDeviceTreeResident(const DialogCaptureContext& context,
                             const char* what) {
  DialogEnvironment& env = context.env;
  if (!FetchDialogNodeResident(env.executor, *env.node_service,
                               scada::devices::id::Devices)) {
    ADD_FAILURE() << what << ": Devices folder not found";
    return false;
  }

  const std::vector<scada::NodeId> device_ids =
      EnumerateDeviceIds(*env.node_service);
  if (device_ids.empty()) {
    ADD_FAILURE() << what << ": no devices under the Devices folder";
    return false;
  }
  if (!FetchNodesResident(env.executor, *env.node_service, device_ids)) {
    ADD_FAILURE() << what << ": failed to fetch devices";
    return false;
  }

  std::vector<scada::NodeId> nested_ids;
  for (const scada::NodeId& id : EnumerateDeviceIds(*env.node_service)) {
    if (std::ranges::find(device_ids, id) == device_ids.end())
      nested_ids.push_back(id);
  }
  if (!nested_ids.empty() &&
      !FetchNodesResident(env.executor, *env.node_service, nested_ids)) {
    ADD_FAILURE() << what << ": failed to fetch nested devices";
    return false;
  }
  return true;
}

}  // namespace

bool CaptureChangePasswordDialog(const DialogCaptureContext& context) {
  // Set Password for the fixture administrator (USER.5, the same user the
  // users-rbac capture shows). LocalEvents only collects the (never-fired)
  // completion toast.
  DialogEnvironment& env = context.env;
  if (!env.node_service || !env.profile) {
    ADD_FAILURE() << "ChangePasswordDialog needs node_service + profile";
    return false;
  }
  const scada::NodeId user_id = NodeIdFromScadaString("USER.5");
  if (!FetchDialogNodeResident(env.executor, *env.node_service, user_id)) {
    ADD_FAILURE() << "ChangePasswordDialog: fixture user not found";
    return false;
  }
  LocalEvents local_events;
  ShowChangePasswordDialog(
      context.dialog_service,
      ChangePasswordContext{.user_ = env.node_service->GetNode(user_id),
                            .executor_ = env.executor,
                            .local_events_ = local_events,
                            .profile_ = *env.profile});
  return GrabShownDialog(context);
}

bool CaptureMultiCreateDialog(const DialogCaptureContext& context) {
  // Bulk TS/TI creation under the DataItems root. The device combo fills from
  // the fixture's Devices folder; the insert path is never taken.
  DialogEnvironment& env = context.env;
  if (!env.node_service) {
    ADD_FAILURE() << "MultiCreateDialog needs a node_service";
    return false;
  }
  if (!FetchDialogNodeResident(env.executor, *env.node_service,
                               scada::devices::id::Devices)) {
    ADD_FAILURE() << "MultiCreateDialog: Devices folder not found";
    return false;
  }
  ShowMultiCreateDialog(context.dialog_service,
                        MultiCreateContext{*env.node_service,
                                           context.task_manager,
                                           scada::data_items::id::DataItems});
  return GrabShownDialog(context);
}

bool CaptureCreateServiceItemDialog(const DialogCaptureContext& context) {
  // Service-object creation under the DataItems root - the modal the object
  // tree opens as «Создание сервисных объектов». The device combo fills from
  // the fixture's Devices folder and the component list from the selected
  // device's data variables; the insert path (PostInsertTask) is never taken.
  constexpr const char* kWhat = "CreateServiceItemDialog";
  DialogEnvironment& env = context.env;
  if (!env.node_service) {
    ADD_FAILURE() << kWhat << " needs a node_service";
    return false;
  }
  if (!FetchDeviceTreeResident(context, kWhat))
    return false;

  ShowCreateServiceItemDialog(
      context.dialog_service,
      CreateServiceItemContext{
          .node_service_ = *env.node_service,
          .task_manager_ = context.task_manager,
          .parent_id_ = scada::data_items::id::DataItems});
  QApplication::processEvents();

  // Point the combo at the RTU with a full set of diagnostic variables. The
  // dialog opens on whichever device sorts first, and that is not a fixed
  // device: since the -104 RTU was parented under its link (2026-08-23) its
  // name is link-qualified, which sorts it below a device carrying a single
  // `Online` — so the image documented "mirror the device's components" with a
  // one-row list. Choosing the device here keeps what the image teaches
  // independent of how the fixture's names happen to collate.
  if (!SelectDeviceInDialogCombo(context.spec, u"КП-01 МЭК-60870"))
    return false;
  if (!ReportIfDialogListEmpty(context.spec))
    return false;
  return GrabAndCloseVisibleDialogOrReport(context.spec,
                                           context.publish_guard);
}

}  // namespace scada::screenshot_generator
