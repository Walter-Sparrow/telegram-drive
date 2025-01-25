#include "utils.h"
#include <iostream>
#include <fcntl.h>
#include <corecrt_io.h>

const wchar_t *DirectoryPath = L"C:\\TelegramDrive";

int wmain(void)
{
    _setmode(_fileno(stdout), _O_U16TEXT);

    if (!CreateDirectoryW(DirectoryPath, NULL))
    {
        if (GetLastError() != ERROR_ALREADY_EXISTS)
        {
            std::cerr << "Failed to create directory: " << GetLastError() << std::endl;
            return 1;
        }
    }

    HANDLE DirectoryHandle = CreateFileW(
        DirectoryPath,
        FILE_LIST_DIRECTORY,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
        NULL);

    if (DirectoryHandle == INVALID_HANDLE_VALUE)
    {
        std::cerr << "Failed to create directory: " << GetLastError() << std::endl;
        return 1;
    }

    std::wcout << L"Monitoring directory: " << DirectoryPath << "\n";

    const DWORD BufferSize = 1024;
    BYTE Buffer[BufferSize];
    DWORD BytesReturned;

    while (ReadDirectoryChangesW(
        DirectoryHandle,
        Buffer,
        BufferSize,
        FALSE,
        FILE_NOTIFY_CHANGE_FILE_NAME |
            FILE_NOTIFY_CHANGE_DIR_NAME |
            FILE_NOTIFY_CHANGE_ATTRIBUTES |
            FILE_NOTIFY_CHANGE_SIZE |
            FILE_NOTIFY_CHANGE_LAST_WRITE |
            FILE_NOTIFY_CHANGE_CREATION,
        &BytesReturned,
        NULL,
        NULL))
    {
        std::wcout << L"Changes detected" << "\n";
        FILE_NOTIFY_INFORMATION *Notification = (FILE_NOTIFY_INFORMATION *)Buffer;
        do
        {
            std::wstring Action = GetAction(Notification->Action);
            std::wstring FileName = GetFileName(Notification);

            std::wcout << L"File: " << FileName << "\n";
            std::wcout << L"Action: " << Action << "\n";

            Notification = NextNotification(Notification);
        } while (Notification);
    }

    std::wcerr << L"Failed to monitor directory: " << GetLastError() << std::endl;
    return 0;
}
