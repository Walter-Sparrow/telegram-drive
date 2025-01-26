#ifndef MONITOR_H
#define MONITOR_H

#include <iostream>
#include <string>
#include <windows.h>

FILE_NOTIFY_INFORMATION *
NextNotification(FILE_NOTIFY_INFORMATION *Notification);

std::wstring
GetFileName(FILE_NOTIFY_INFORMATION *Notification);

std::wstring
GetAction(DWORD Action);

#endif
