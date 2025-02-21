#include <windows.h>
#include <cfapi.h>
#include <corecrt_io.h>
#include <fcntl.h>
#include <iostream>
#include <thread>
#include <utils.h>

#pragma comment(lib, "CldApi.lib")

const wchar_t *DirectoryPath = L"C:\\TelegramDrive";
CF_CONNECTION_KEY ConnectionKey = {0};

bool RegisterSyncRoot() {
  CF_SYNC_REGISTRATION Reg = {};
  Reg.StructSize = sizeof(Reg);
  Reg.ProviderName = L"TelegramDrive";
  Reg.ProviderVersion = L"1.0";

  GUID ProviderGuid = {0xa2572f01,
                       0x5f92,
                       0x4d7d,
                       {0xaa, 0xe0, 0x31, 0xfa, 0xb4, 0x39, 0x10, 0x30}};
  Reg.ProviderId = ProviderGuid;

  CF_SYNC_POLICIES Policies = {};
  Policies.StructSize = sizeof(Policies);
  Policies.HardLink = CF_HARDLINK_POLICY_NONE;

  DWORD RegisterFlags = CF_REGISTER_FLAG_DISABLE_ON_DEMAND_POPULATION_ON_ROOT;
  HRESULT Result = CfRegisterSyncRoot(DirectoryPath, &Reg, &Policies,
                                      (CF_REGISTER_FLAGS)RegisterFlags);

  if (FAILED(Result)) {
    OutputDebugStringW(L"[RegisterSyncRoot] failed\n");
    return false;
  }

  OutputDebugStringW(L"[RegisterSyncRoot] succeeded\n");
  return true;
}

bool ConnectSyncRoot() {
  CF_CALLBACK_REGISTRATION Callbacks[] = {{CF_CALLBACK_TYPE_NONE, nullptr}};
  CF_CONNECT_FLAGS ConnectFlags = CF_CONNECT_FLAG_NONE;
  HRESULT Result = CfConnectSyncRoot(DirectoryPath, Callbacks, nullptr,
                                     ConnectFlags, &ConnectionKey);

  if (FAILED(Result)) {
    OutputDebugStringW(L"[ConnectSyncRoot] failed\n");
    return false;
  }

  OutputDebugStringW(L"[ConnectSyncRoot] succeeded\n");
  return true;
}

void DisconnectSyncRoot() {
  HRESULT Result = CfDisconnectSyncRoot(ConnectionKey);
  if (FAILED(Result)) {
    OutputDebugStringW(L"[DisconnectSyncRoot] failed\n");
  } else {
    OutputDebugStringW(L"[DisconnectSyncRoot] succeeded\n");
  }
  ConnectionKey.Internal = 0;
}

void UnregisterSyncRoot() {
  HRESULT Result = CfUnregisterSyncRoot(DirectoryPath);
  if (FAILED(Result)) {
    OutputDebugStringW(L"[UnregisterSyncRoot] failed\n");
  } else {
    OutputDebugStringW(L"[UnregisterSyncRoot] succeeded\n");
  }
}

bool CreateHelloWorldPlaceholder(int i) {
  CF_PLACEHOLDER_CREATE_INFO Placeholder = {};

  wchar_t FileName[MAX_PATH];
  swprintf(FileName, MAX_PATH, L"HelloWorld_%d.txt", i);

  Placeholder.RelativeFileName = FileName;
  Placeholder.FsMetadata.FileSize.QuadPart = 0;
  Placeholder.Flags = CF_PLACEHOLDER_CREATE_FLAG_NONE;

  static const char Id[] = "HelloWorldID";
  Placeholder.FileIdentity = Id;
  Placeholder.FileIdentityLength = (DWORD)sizeof(Id);

  HRESULT Result = CfCreatePlaceholders(DirectoryPath, &Placeholder, 1,
                                        CF_CREATE_FLAG_NONE, nullptr);

  if (FAILED(Result)) {
    OutputDebugStringW(L"[CreateHelloWorldPlaceholder] failed\n");
    return false;
  }

  OutputDebugStringW(L"[CreateHelloWorldPlaceholder] succeeded\n");
  return true;
}

DWORD WINAPI MonitorDirectory(LPVOID Param) {
  HANDLE Handle = (HANDLE)Param;
  const DWORD BufferSize = 1024;
  BYTE Buffer[BufferSize];
  DWORD BytesReturned;

  while (ReadDirectoryChangesW(
      Handle, Buffer, BufferSize, FALSE,
      FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
          FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
          FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
      &BytesReturned, NULL, NULL)) {
    OutputDebugStringW(L"[ReadDirectoryChanges] change detected\n");
    FILE_NOTIFY_INFORMATION *Notification = (FILE_NOTIFY_INFORMATION *)Buffer;
    do {
      const wchar_t *Action = GetAction(Notification->Action);
      const wchar_t *FileName = Notification->FileName;

      wchar_t buffer[MAX_PATH];
      swprintf(buffer, MAX_PATH, L"File: %.*s\n",
               int(Notification->FileNameLength / sizeof(wchar_t)), FileName);
      OutputDebugStringW(buffer);

      swprintf(buffer, MAX_PATH, L"Action: %s\n", Action);
      OutputDebugStringW(buffer);

      Notification = NextNotification(Notification);
    } while (Notification);
  }

  return 0;
}

DWORD WINAPI TestPlaceholderLoop(LPVOID /* Param */) {
  for (int i = 0; i < 100; i++) {
    CreateHelloWorldPlaceholder(i);
    Sleep(1000);
  }

  return 0;
}

int wmain(void) {
  _setmode(_fileno(stdout), _O_U16TEXT);

  if (!CreateDirectoryW(DirectoryPath, NULL)) {
    if (GetLastError() != ERROR_ALREADY_EXISTS) {
      OutputDebugStringW(L"[CreateDirectory] failed\n");
      return 1;
    }
  }

  HANDLE DirectoryHandle = CreateFileW(
      DirectoryPath, FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

  if (DirectoryHandle == INVALID_HANDLE_VALUE) {
    OutputDebugStringW(L"[CreateFile] failed\n");
    return 1;
  }

  if (!RegisterSyncRoot()) {
    return 1;
  }

  if (!ConnectSyncRoot()) {
    UnregisterSyncRoot();
    return 1;
  }

  OutputDebugStringW(L"[Monitoring directory]\n");
  HANDLE MonitorHandle =
      CreateThread(NULL, 0, MonitorDirectory, (LPVOID)DirectoryHandle, 0, NULL);

  HANDLE TestPlaceholdersHandle =
      CreateThread(NULL, 0, TestPlaceholderLoop, NULL, 0, NULL);

  HANDLE Handles[] = {MonitorHandle, TestPlaceholdersHandle};
  WaitForMultipleObjects(2, Handles, TRUE, INFINITE);

  return 0;
}
