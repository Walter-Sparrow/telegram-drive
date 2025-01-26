#include <utils.h>
#include <cfapi.h>
#include <corecrt_io.h>
#include <fcntl.h>
#include <iostream>
#include <thread>

#pragma comment(lib, "CldApi.lib")

const wchar_t *DirectoryPath = L"C:\\TelegramDrive";
CF_CONNECTION_KEY ConnectionKey = {0};

bool RegisterSyncRoot()
{
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

  if (FAILED(Result))
  {
    std::wcerr << L"[RegisterSyncRoot] failed, hr=0x" << std::hex << Result
               << std::endl;
    return false;
  }

  std::wcout << L"[INFO] Sync root registered (root on-demand enumeration "
                L"disabled).\n";
  return true;
}

bool ConnectSyncRoot()
{
  CF_CALLBACK_REGISTRATION Callbacks[] = {{CF_CALLBACK_TYPE_NONE, nullptr}};

  CF_CONNECT_FLAGS ConnectFlags = CF_CONNECT_FLAG_NONE;

  HRESULT Result = CfConnectSyncRoot(DirectoryPath, Callbacks, nullptr,
                                     ConnectFlags, &ConnectionKey);

  if (FAILED(Result))
  {
    std::wcerr << L"[ConnectSyncRoot] failed, hr=0x" << std::hex << Result
               << std::endl;
    return false;
  }

  std::wcout << L"[INFO] Connected to sync root. CF_CONNECTION_KEY acquired.\n";
  return true;
}

void DisconnectSyncRoot()
{
  HRESULT Result = CfDisconnectSyncRoot(ConnectionKey);
  if (FAILED(Result))
  {
    std::wcerr << L"[DisconnectSyncRoot] failed, hr=0x" << std::hex << Result
               << std::endl;
  }
  else
  {
    std::wcout << L"[INFO] Disconnected sync root.\n";
  }
  ConnectionKey.Internal = 0;
}

void UnregisterSyncRoot()
{
  HRESULT Result = CfUnregisterSyncRoot(DirectoryPath);

  if (FAILED(Result))
  {
    std::wcerr << L"[UnregisterSyncRoot] failed, hr=0x" << std::hex << Result
               << std::endl;
  }
  else
  {
    std::wcout << L"[INFO] Sync root unregistered.\n";
  }
}

bool CreateHelloWorldPlaceholder(int i)
{
  CF_PLACEHOLDER_CREATE_INFO Placeholder = {};

  std::wstring FileName = L"HelloWorld";
  FileName.append(std::to_wstring(i));
  FileName.append(L".txt");

  Placeholder.RelativeFileName = FileName.c_str();
  Placeholder.FsMetadata.FileSize.QuadPart = 0;
  Placeholder.Flags = CF_PLACEHOLDER_CREATE_FLAG_NONE;

  static const char Id[] = "HelloWorldID";
  Placeholder.FileIdentity = Id;
  Placeholder.FileIdentityLength = (DWORD)sizeof(Id);

  HRESULT Result = CfCreatePlaceholders(DirectoryPath, &Placeholder, 1,
                                        CF_CREATE_FLAG_NONE, nullptr);

  if (FAILED(Result))
  {
    std::wcerr
        << L"[CreateHelloWorldPlaceholder] CfCreatePlaceholders failed, hr=0x"
        << std::hex << Result << std::endl;
    return false;
  }

  std::wcout << L"[INFO] Placeholder " << Placeholder.RelativeFileName
             << " created.\n";
  return true;
}

int wmain(void)
{
  _setmode(_fileno(stdout), _O_U16TEXT);

  if (!CreateDirectoryW(DirectoryPath, NULL))
  {
    if (GetLastError() != ERROR_ALREADY_EXISTS)
    {
      std::cerr << "Failed to create directory: " << GetLastError()
                << std::endl;
      return 1;
    }
  }

  HANDLE DirectoryHandle = CreateFileW(
      DirectoryPath, FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

  if (DirectoryHandle == INVALID_HANDLE_VALUE)
  {
    std::cerr << "Failed to create directory: " << GetLastError() << std::endl;
    return 1;
  }

  if (!RegisterSyncRoot())
  {
    return 1;
  }

  if (!ConnectSyncRoot())
  {
    UnregisterSyncRoot();
    return 1;
  }

  std::thread PlaceholderThread([]
                                {
    for (int i = 0; i < 10; i++) {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      if (!CreateHelloWorldPlaceholder(i)) {
        DisconnectSyncRoot();
        UnregisterSyncRoot();
      }
    } });

  std::wcout << L"Monitoring directory: " << DirectoryPath << "\n";

  const DWORD BufferSize = 1024;
  BYTE Buffer[BufferSize];
  DWORD BytesReturned;

  while (ReadDirectoryChangesW(
      DirectoryHandle, Buffer, BufferSize, FALSE,
      FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
          FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
          FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
      &BytesReturned, NULL, NULL))
  {
    std::wcout << L"Changes detected\n";
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

  PlaceholderThread.join();
  return 0;
}
