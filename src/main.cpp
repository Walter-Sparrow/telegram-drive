#include <windows.h>
#include <cfapi.h>
#include <corecrt_io.h>
#include <fcntl.h>
#include <iostream>
#include <thread>
#include <utils.h>
#include <vector>

#pragma comment(lib, "CldApi.lib")

const wchar_t *directory_path = L"C:\\TelegramDrive";
CF_CONNECTION_KEY connection_key = {0};

bool register_sync_root() {
  CF_SYNC_REGISTRATION reg = {};
  reg.StructSize = sizeof(reg);
  reg.ProviderName = L"TelegramDrive";
  reg.ProviderVersion = L"1.0";

  GUID provider_guid = {0xa2572f01,
                        0x5f92,
                        0x4d7d,
                        {0xaa, 0xe0, 0x31, 0xfa, 0xb4, 0x39, 0x10, 0x30}};
  reg.ProviderId = provider_guid;

  CF_SYNC_POLICIES policies = {};
  policies.StructSize = sizeof(policies);
  policies.HardLink = CF_HARDLINK_POLICY_NONE;

  DWORD RegisterFlags = CF_REGISTER_FLAG_DISABLE_ON_DEMAND_POPULATION_ON_ROOT |
                        CF_REGISTER_FLAG_UPDATE;
  HRESULT Result = CfRegisterSyncRoot(directory_path, &reg, &policies,
                                      (CF_REGISTER_FLAGS)RegisterFlags);

  if (FAILED(Result)) {
    wprintf(L"[register_sync_root] failed: 0x%08X\n", Result);
    return false;
  }

  wprintf(L"[register_sync_root] succeeded\n");
  return true;
}

void CALLBACK on_fetch_data(const CF_CALLBACK_INFO *callback_info,
                            const CF_CALLBACK_PARAMETERS *callback_parameters) {
  LARGE_INTEGER request_offset =
      callback_parameters->FetchData.RequiredFileOffset;
  LARGE_INTEGER request_length = callback_parameters->FetchData.RequiredLength;

  BYTE *buffer = (BYTE *)malloc(request_length.QuadPart);
  for (int i = 0; i < request_length.QuadPart && i < 4; ++i) {
    buffer[i] = "Test"[i % 4];
  }

  CF_OPERATION_INFO op_info = {};
  CF_OPERATION_PARAMETERS op_params = {};

  op_info.StructSize = sizeof(op_info);
  op_info.Type = CF_OPERATION_TYPE_TRANSFER_DATA;
  op_info.ConnectionKey = callback_info->ConnectionKey;
  op_info.TransferKey = callback_info->TransferKey;

  op_params.ParamSize = sizeof(op_params);
  op_params.TransferData.CompletionStatus = S_OK;
  op_params.TransferData.Offset = request_offset;
  op_params.TransferData.Length = request_length;
  op_params.TransferData.Buffer = buffer;

  HRESULT hr = CfExecute(&op_info, &op_params);
  if (FAILED(hr)) {
    wprintf(L"[on_fetch_data] CfExecute failed: 0x%08X\n", hr);
  }

  free(buffer);
}

bool connect_sync_root() {
  CF_CALLBACK_REGISTRATION callbacks[] = {
      {CF_CALLBACK_TYPE_FETCH_DATA, on_fetch_data},
      CF_CALLBACK_REGISTRATION_END};
  CF_CONNECT_FLAGS connect_flags = CF_CONNECT_FLAG_NONE;
  HRESULT hr = CfConnectSyncRoot(directory_path, callbacks, NULL, connect_flags,
                                 &connection_key);

  if (FAILED(hr)) {
    wprintf(L"[connect_sync_root] failed: 0x%08X\n", hr);
    return false;
  }

  wprintf(L"[connect_sync_root] succeeded\n");
  return true;
}

void disconnect_sync_root() {
  HRESULT hr = CfDisconnectSyncRoot(connection_key);
  if (FAILED(hr)) {
    wprintf(L"[disconnect_sync_root] failed: 0x%08X\n", hr);
  } else {
    wprintf(L"[disconnect_sync_root] succeeded\n");
  }
  connection_key.Internal = 0;
}

void unregister_sync_root() {
  HRESULT hr = CfUnregisterSyncRoot(directory_path);
  if (FAILED(hr)) {
    wprintf(L"[unregister_sync_root] failed: 0x%08X\n", hr);
  } else {
    wprintf(L"[unregister_sync_root] succeeded\n");
  }
}

DWORD WINAPI monitor_directory(LPVOID param) {
  HANDLE handle = (HANDLE)param;
  const DWORD buffer_size = 1024;
  BYTE buffer[buffer_size];
  DWORD bytes_returned;

  wprintf(L"Monitoring directory...\n");
  while (ReadDirectoryChangesW(
      handle, buffer, buffer_size, FALSE,
      FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME |
          FILE_NOTIFY_CHANGE_ATTRIBUTES | FILE_NOTIFY_CHANGE_SIZE |
          FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION,
      &bytes_returned, NULL, NULL)) {
    wprintf(L"[monitor_directory] change detected\n");
    FILE_NOTIFY_INFORMATION *notification = (FILE_NOTIFY_INFORMATION *)buffer;
    do {
      const wchar_t *action = GetAction(notification->Action);
      const wchar_t *file_name = notification->FileName;
      int file_name_length =
          int(notification->FileNameLength / sizeof(wchar_t));

      wprintf(L"[monitor_directory] action: \"%s\" on file: \"%.*s\"\n", action,
              file_name_length, file_name);

      wchar_t file_path[MAX_PATH];
      swprintf(file_path, MAX_PATH, L"%s\\%.*s", directory_path,
               file_name_length, file_name);

      if (notification->Action == FILE_ACTION_ADDED) {
        HANDLE file_handle = CreateFileW(file_path, 0, FILE_READ_DATA, NULL,
                                         OPEN_EXISTING, 0, NULL);

        BYTE *file_identity = (BYTE *)file_path;
        DWORD file_identity_length = (DWORD)wcslen(file_path) * sizeof(wchar_t);

        HRESULT hr = CfConvertToPlaceholder(
            file_handle, file_identity, file_identity_length,
            CF_CONVERT_FLAG_DEHYDRATE | CF_CONVERT_FLAG_MARK_IN_SYNC |
                CF_CONVERT_FLAG_FORCE_CONVERT_TO_CLOUD_FILE,
            NULL, NULL);

        if (FAILED(hr)) {
          wprintf(
              L"[monitor_directory] CfConvertToPlaceholder failed: 0x%08X\n",
              hr);
        } else {
          wprintf(L"[monitor_directory] CfConvertToPlaceholder succeeded\n");
        }

        CloseHandle(file_handle);
      }

      notification = NextNotification(notification);
    } while (notification);
  }

  return 0;
}

int wmain(void) {
  _setmode(_fileno(stdout), _O_U16TEXT);

  if (!CreateDirectoryW(directory_path, NULL)) {
    DWORD error = GetLastError();
    if (error != ERROR_ALREADY_EXISTS) {
      wprintf(L"[CreateDirectory] failed: 0x%08X\n", error);
      return 1;
    }
  }

  HANDLE directory_handle = CreateFileW(
      directory_path, FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
      OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);

  if (directory_handle == INVALID_HANDLE_VALUE) {
    wprintf(L"[CreateFile] failed: 0x%08X\n", GetLastError());
    return 1;
  }

  if (!register_sync_root()) {
    return 1;
  }

  if (!connect_sync_root()) {
    unregister_sync_root();
    return 1;
  }

  HANDLE monitor_handle = CreateThread(NULL, 0, monitor_directory,
                                       (LPVOID)directory_handle, 0, NULL);

  HANDLE handles[] = {monitor_handle};
  WaitForMultipleObjects(1, handles, TRUE, INFINITE);

  return 0;
}
