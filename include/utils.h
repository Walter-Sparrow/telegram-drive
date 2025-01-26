#ifndef MONITOR_H
#define MONITOR_H

#include <windows.h>
#include <iostream>
#include <string>

FILE_NOTIFY_INFORMATION *
NextNotification(FILE_NOTIFY_INFORMATION *Notification);

std::wstring GetFileName(FILE_NOTIFY_INFORMATION *Notification);
std::wstring GetAction(DWORD Action);

#endif
