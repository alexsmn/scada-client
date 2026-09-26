#include "services/core_ui_text.h"

#include "scada/qualifier.h"
#include "scada/variant.h"

#include "aui/translation.h"

#include <QtGlobal>

namespace {

struct StatusTextEntry {
  scada::StatusCode code;
  // English source; the lookup key in `client_ru.ts`'s empty context, which is
  // the context `Translate()` reads.
  const char* text;
};

constexpr StatusTextEntry kStatusTexts[] = {
    {scada::StatusCode::Good,
     QT_TRANSLATE_NOOP("", "Operation completed successfully")},
    {scada::StatusCode::Good_Pending,
     QT_TRANSLATE_NOOP("", "Operation in progress")},
    {scada::StatusCode::Uncertain_StateWasNotChanged,
     QT_TRANSLATE_NOOP("", "The lock was not changed")},
    {scada::StatusCode::Bad, QT_TRANSLATE_NOOP("", "Error")},
    {scada::StatusCode::Bad_WrongLoginCredentials,
     QT_TRANSLATE_NOOP("", "Wrong user name or password")},
    {scada::StatusCode::Bad_UserIsAlreadyLoggedOn,
     QT_TRANSLATE_NOOP("", "A session for this user is already open")},
    {scada::StatusCode::Bad_UnsupportedProtocolVersion,
     QT_TRANSLATE_NOOP("", "Protocol version is not supported")},
    {scada::StatusCode::Bad_ObjectIsBusy,
     QT_TRANSLATE_NOOP("", "Another command is already running")},
    {scada::StatusCode::Bad_WrongNodeId,
     QT_TRANSLATE_NOOP("", "Wrong node identifier")},
    {scada::StatusCode::Bad_WrongDeviceId,
     QT_TRANSLATE_NOOP("", "Wrong device identifier")},
    {scada::StatusCode::Bad_Disconnected,
     QT_TRANSLATE_NOOP("", "Not connected")},
    {scada::StatusCode::Bad_SessionForcedLogoff,
     QT_TRANSLATE_NOOP("", "Session closed because this user connected again")},
    {scada::StatusCode::Bad_Timeout,
     QT_TRANSLATE_NOOP("", "Operation aborted after the wait timed out")},
    {scada::StatusCode::Bad_CantDeleteDependentNode,
     QT_TRANSLATE_NOOP(
         "",
         "Cannot delete the object because dependent objects exist")},
    {scada::StatusCode::Bad_ServerWasShutDown,
     QT_TRANSLATE_NOOP("", "Session closed because the server stopped")},
    {scada::StatusCode::Bad_WrongMethodId,
     QT_TRANSLATE_NOOP("", "The command is not supported by this object")},
    {scada::StatusCode::Bad_CantDeleteOwnUser,
     QT_TRANSLATE_NOOP("",
                       "Cannot delete a user from a session that user opened")},
    {scada::StatusCode::Bad_DuplicateNodeId,
     QT_TRANSLATE_NOOP("", "An object with this identifier already exists")},
    {scada::StatusCode::Bad_UnsupportedFileVersion,
     QT_TRANSLATE_NOOP("", "File version is not supported")},
    {scada::StatusCode::Bad_WrongTypeId,
     QT_TRANSLATE_NOOP("", "Wrong object type")},
    {scada::StatusCode::Bad_WrongParentId,
     QT_TRANSLATE_NOOP("", "Wrong parent object identifier")},
    {scada::StatusCode::Bad_SessionIsLoggedOff,
     QT_TRANSLATE_NOOP("", "Not logged on")},
    {scada::StatusCode::Bad_WrongSubscriptionId,
     QT_TRANSLATE_NOOP("", "Wrong subscription number")},
    {scada::StatusCode::Bad_WrongIndex, QT_TRANSLATE_NOOP("", "Wrong index")},
    {scada::StatusCode::Bad_Iec60870UnknownType,
     QT_TRANSLATE_NOOP("", "Wrong IEC 60870-5 ASDU type")},
    {scada::StatusCode::Bad_Iec60870UnknownCot,
     QT_TRANSLATE_NOOP("", "Wrong IEC 60870-5 cause of transmission")},
    {scada::StatusCode::Bad_Iec60870UnknownDevice,
     QT_TRANSLATE_NOOP("", "Wrong IEC 60870-5 device address")},
    {scada::StatusCode::Bad_Iec60870UnknownAddress,
     QT_TRANSLATE_NOOP("", "Wrong IEC 60870-5 information object address")},
    {scada::StatusCode::Bad_Iec60870UnknownError,
     QT_TRANSLATE_NOOP("", "IEC 60870-5 protocol error")},
    {scada::StatusCode::Bad_WrongCallArguments,
     QT_TRANSLATE_NOOP("", "Wrong command arguments")},
    {scada::StatusCode::Bad_CantParseString,
     QT_TRANSLATE_NOOP("",
                       "Cannot convert the string to a value of this type")},
    {scada::StatusCode::Bad_TooLongString,
     QT_TRANSLATE_NOOP("", "String is too long")},
    {scada::StatusCode::Bad_WrongPropertyId,
     QT_TRANSLATE_NOOP("", "Wrong object attribute")},
    {scada::StatusCode::Bad_WrongReferenceId,
     QT_TRANSLATE_NOOP("", "Wrong reference type")},
    {scada::StatusCode::Bad_WrongNodeClass,
     QT_TRANSLATE_NOOP("", "Wrong node class")},
    {scada::StatusCode::Bad_WrongAttributeId,
     QT_TRANSLATE_NOOP("", "Wrong attribute")},
    {scada::StatusCode::Bad_Iec61850Error,
     QT_TRANSLATE_NOOP("", "IEC 61850 protocol error")},
    {scada::StatusCode::Bad_NothingToDo,
     QT_TRANSLATE_NOOP("", "The request is empty")},
    {scada::StatusCode::Bad_BrowseNameInvalid,
     QT_TRANSLATE_NOOP("", "Name not found")},
    {scada::StatusCode::Bad_WrongTargetId,
     QT_TRANSLATE_NOOP("", "Wrong reference target")},
    {scada::StatusCode::Bad_MonitoredItemIdInvalid,
     QT_TRANSLATE_NOOP("", "Wrong monitored item number")},
    {scada::StatusCode::Bad_MessageNotAvailable,
     QT_TRANSLATE_NOOP("", "The requested message is no longer available")},
    {scada::StatusCode::Bad_ApplicationSignatureInvalid,
     QT_TRANSLATE_NOOP("", "Invalid client application signature")},
    {scada::StatusCode::Bad_TooManyOperations,
     QT_TRANSLATE_NOOP("", "Too many operations in the request")},
    {scada::StatusCode::Bad_TooManyMonitoredItems,
     QT_TRANSLATE_NOOP("", "Too many monitored items in the request")},
    {scada::StatusCode::Bad_SequenceNumberUnknown,
     QT_TRANSLATE_NOOP("", "Unknown message sequence number")},
    {scada::StatusCode::Bad_NoContinuationPoints,
     QT_TRANSLATE_NOOP("", "The browse continuation point limit is exhausted")},
    {scada::StatusCode::Bad_TimestampsToReturnInvalid,
     QT_TRANSLATE_NOOP("", "Wrong TimestampsToReturn value")},
    {scada::StatusCode::Bad_ViewIdUnknown,
     QT_TRANSLATE_NOOP("", "Unknown view identifier")},
    {scada::StatusCode::Bad_HistoryOperationInvalid,
     QT_TRANSLATE_NOOP("", "Invalid history request parameters")},
    {scada::StatusCode::Bad_NoSubscription,
     QT_TRANSLATE_NOOP("", "The session has no subscriptions")},
    {scada::StatusCode::Bad_UserAccessDenied,
     QT_TRANSLATE_NOOP("", "Not enough rights to perform the operation")},
    {scada::StatusCode::Bad_NotSupported,
     QT_TRANSLATE_NOOP("", "Operation is not supported")},
    {scada::StatusCode::Bad_LicenseExpired,
     QT_TRANSLATE_NOOP("", "The license has expired")},
    {scada::StatusCode::Bad_WaitingForInitialData,
     QT_TRANSLATE_NOOP("", "No value received from the data source yet")},
    {scada::StatusCode::Bad_OutOfRange,
     QT_TRANSLATE_NOOP("", "The value is out of range and will not be stored")},
    {scada::StatusCode::Bad_NotWritable,
     QT_TRANSLATE_NOOP("", "The value cannot be written")},
    {scada::StatusCode::Bad_ResponseTooLarge,
     QT_TRANSLATE_NOOP("", "The response is too large to send")},
    {scada::StatusCode::Bad_InvalidState,
     QT_TRANSLATE_NOOP(
         "",
         "The object is not in a state that allows this operation")},
    {scada::StatusCode::Bad_NotReadable,
     QT_TRANSLATE_NOOP("", "The value cannot be read")},
};

struct QualifierFlagTextEntry {
  unsigned flag;
  // English source, keyed like the status sentences. These are deliberately
  // short: the rendering is a space-separated run inside a narrow grid cell.
  //
  // "Bad quality" and "No link" are spelled that way because `Translate()`
  // looks up by source text with no disambiguation context, and the catalog
  // already maps "Bad" to "Недостоверно" and "Offline" to "Нет связи" — the
  // long forms used elsewhere. Reusing those sources would have silently
  // widened this strip.
  const char* text;
};

constexpr QualifierFlagTextEntry kQualifierFlagTexts[] = {
    {scada::Qualifier::BAD, QT_TRANSLATE_NOOP("", "Bad quality")},
    {scada::Qualifier::BACKUP, QT_TRANSLATE_NOOP("", "Backup")},
    {scada::Qualifier::OFFLINE, QT_TRANSLATE_NOOP("", "No link")},
    {scada::Qualifier::MANUAL, QT_TRANSLATE_NOOP("", "Manual")},
    {scada::Qualifier::MISCONFIGURED, QT_TRANSLATE_NOOP("", "Misconfigured")},
    {scada::Qualifier::SIMULATED, QT_TRANSLATE_NOOP("", "Simulated")},
    {scada::Qualifier::SPORADIC, QT_TRANSLATE_NOOP("", "Sporadic")},
    {scada::Qualifier::STALE, QT_TRANSLATE_NOOP("", "Stale")},
    {scada::Qualifier::FAILED, QT_TRANSLATE_NOOP("", "Failed")},
};

}  // namespace

std::u16string StatusText(scada::StatusCode status_code) {
  for (const StatusTextEntry& entry : kStatusTexts) {
    if (entry.code == status_code)
      return Translate(entry.text);
  }

  return IsGood(status_code) ? Translate("Operation completed successfully")
                             : Translate("Error");
}

std::u16string QualifierFlagText(unsigned flag) {
  for (const QualifierFlagTextEntry& entry : kQualifierFlagTexts) {
    if (entry.flag == flag)
      return Translate(entry.text);
  }
  return {};
}

std::u16string BooleanText(bool value) {
  return value ? Translate("Yes") : Translate("No");
}

std::u16string FallbackLabelText(FallbackLabel label) {
  switch (label) {
    case FallbackLabel::kDefaultClose:
      return Translate("On");
    case FallbackLabel::kDefaultOpen:
      return Translate("Off");
    case FallbackLabel::kEmptyDisplayName:
    case FallbackLabel::kUnknownDisplayName:
      return Translate("#NAME?");
  }
  return {};
}

void InstallCoreUiText() {
  scada::SetStatusTextProvider(&StatusText);
  scada::SetQualifierFlagTextProvider(&QualifierFlagText);
  scada::SetBooleanTextProvider(&BooleanText);
  SetFallbackLabelProvider(&FallbackLabelText);
}
