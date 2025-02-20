#include "utils.h"

FILE_NOTIFY_INFORMATION *
NextNotification(FILE_NOTIFY_INFORMATION *Notification) {
  if (Notification->NextEntryOffset == 0) {
    return NULL;
  }

  return (FILE_NOTIFY_INFORMATION *)((char *)Notification +
                                     Notification->NextEntryOffset);
}

const wchar_t *GetAction(DWORD Action) {
  switch (Action) {
  case FILE_ACTION_ADDED:
    return L"File added";
  case FILE_ACTION_REMOVED:
    return L"File removed";
  case FILE_ACTION_MODIFIED:
    return L"File modified";
  case FILE_ACTION_RENAMED_OLD_NAME:
    return L"File renamed (old name)";
  case FILE_ACTION_RENAMED_NEW_NAME:
    return L"File renamed (new name)";
  default:
    return L"Unknown action";
  }
}
