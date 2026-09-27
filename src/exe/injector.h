#ifndef INJECTOR_H
#define INJECTOR_H

#include <windows.h>
#include <tlhelp32.h>
#include <string>

inline DWORD get_process_id_by_name(const wchar_t* processName) 
{
    DWORD p_id = 0;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot != INVALID_HANDLE_VALUE) 
    {
        PROCESSENTRY32W process_entry;
        process_entry.dwSize = sizeof(process_entry);
        if (Process32FirstW(snapshot, &process_entry)) 
        {
            do {
                if (_wcsicmp(process_entry.szExeFile, processName) == 0) 
                {
                    p_id = process_entry.th32ProcessID;
                    break;
                }
            } while (Process32NextW(snapshot, &process_entry));
        }
        CloseHandle(snapshot);
    }
    return p_id;
}

inline std::string inject_DLL(const std::string& dll_path) 
{
    DWORD p_id = get_process_id_by_name(L"Le Mans Ultimate.exe");
    if (p_id == 0) 
        return "Failed: 'Le Mans Ultimate.exe' not found. Is the game running?";

    // Request specific required rights instead of raw PROCESS_ALL_ACCESS
    HANDLE h_Process = OpenProcess
    (
        PROCESS_QUERY_INFORMATION | PROCESS_CREATE_THREAD | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, 
        FALSE, 
        p_id
    );
    
    if (!h_Process) 
    {
        DWORD err = GetLastError();
        return "Failed: OpenProcess denied. Error: " + std::to_string(err) + " (Ensure Control Panel is run as Admin!)";
    }

    LPVOID p_Dll_path = VirtualAllocEx(h_Process, NULL, dll_path.length() + 1, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!p_Dll_path) 
    {
        DWORD err = GetLastError();
        CloseHandle(h_Process);
        return "Failed: VirtualAllocEx error: " + std::to_string(err) + " (Anti-cheat/Game security blocking memory allocation)";
    }

    if (!WriteProcessMemory(h_Process, p_Dll_path, (LPVOID)dll_path.c_str(), dll_path.length() + 1, NULL)) 
    {
        DWORD err = GetLastError();
        VirtualFreeEx(h_Process, p_Dll_path, 0, MEM_RELEASE);
        CloseHandle(h_Process);
        return "Failed: WriteProcessMemory error: " + std::to_string(err);
    }

    LPTHREAD_START_ROUTINE pLoadLibrary = (LPTHREAD_START_ROUTINE)GetProcAddress(
        GetModuleHandleA("kernel32.dll"), "LoadLibraryA"
    );

    if (!pLoadLibrary) 
    {
        VirtualFreeEx(h_Process, p_Dll_path, 0, MEM_RELEASE);
        CloseHandle(h_Process);
        return "Failed: Could not resolve LoadLibraryA address.";
    }

    HANDLE hThread = CreateRemoteThread(h_Process, NULL, 0, pLoadLibrary, p_Dll_path, 0, NULL);
    if (!hThread) 
    {
        DWORD err = GetLastError();
        VirtualFreeEx(h_Process, p_Dll_path, 0, MEM_RELEASE);
        CloseHandle(h_Process);
        return "Failed: CreateRemoteThread error: " + std::to_string(err);
    }

    WaitForSingleObject(hThread, 2000);
    CloseHandle(hThread);
    CloseHandle(h_Process);

    return "Success: Successfully Injected!";
}

#endif