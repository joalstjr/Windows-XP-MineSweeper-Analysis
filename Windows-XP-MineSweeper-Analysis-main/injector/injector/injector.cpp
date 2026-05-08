#include <Windows.h>
#include <TlHelp32.h>
#include <iostream>

static DWORD GetProcessByName(const char* lpProcessName)
{
    char lpCurrentProcessName[255];

    PROCESSENTRY32 ProcList{};
    ProcList.dwSize = sizeof(ProcList);

    // CreateToolhelp32Snapshot: 현재 시스템에서 실행중인 프로세스의 snapshot handle을 반환하는 함수
    const HANDLE hProcList = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcList == INVALID_HANDLE_VALUE)
        return -1;

    // Process32First: 프로세스의 snapshot handle에서 process list를 읽어옴
    if (!Process32First(hProcList, &ProcList))
        return -1;
    // wcstombs_s: 와이드 문자의 시퀀스를 멀티 바이트 문자의 시퀀스로 변환
    wcstombs_s(nullptr, lpCurrentProcessName, ProcList.szExeFile, 255);

    if (lstrcmpA(lpCurrentProcessName, lpProcessName) == 0)
        return ProcList.th32ProcessID;

    // Process32Next: 시스템 snapshot에 기록된 다음 프로세스에 대한 정보를 반환
    // 목표 프로세스를 탐색후 발견되면 PID를 반환
    while (Process32Next(hProcList, &ProcList))
    {
        wcstombs_s(nullptr, lpCurrentProcessName, ProcList.szExeFile, 255);

        if (lstrcmpA(lpCurrentProcessName, lpProcessName) == 0)
            return ProcList.th32ProcessID;
    }
    return -1;
}

int main(const int argc, char* argv[])
{
    // DLL Name, ProcessName, DLL Path
    char* lpDLLName;
    char* lpProcessName;
    char lpFullDLLPath[MAX_PATH];

    // arg 입력 확인 
    if (argc == 3)
    {
        lpDLLName = argv[1];
        lpProcessName = argv[2];
    }
    else
    {
        printf("Input requirements: inject.exe <DLL Path> <Process Name>\n");
        return -1;
    }

    // PID를 ProcessName으로 받아오기
    const DWORD dwProcessID = GetProcessByName(lpProcessName);
    if (dwProcessID == (DWORD)-1)
    {
        printf("An error is occured when trying to find the target process.\n");
        return -1;
    }

    printf("[DLL Injector]\n");
    printf("Process : %s\n", lpProcessName);
    printf("Process ID : %i\n\n", (int)dwProcessID);

    // GetFullPathNameA: 지정된 파일의 전체 경로 및 이름 검색
    // 반환값: lpBuffer에 전체 경로를 저장 
    const DWORD dwFullPathResult = GetFullPathNameA(lpDLLName, MAX_PATH, lpFullDLLPath, nullptr);
    if (dwFullPathResult == 0)
    {
        printf("An error is occured when trying to get the full path of the DLL.\n");
        return -1;
    }

    // OpenProcess: 기존 로컬 프로세스 개체를 여는 함수
    // 반환값: Process Handle
    const HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessID);
    if (hProcess == INVALID_HANDLE_VALUE)
    {
        printf("An error is occured when trying to open the target process.\n");
        return -1;
    }

    printf("Process opened successfully.\n");

    // VirtualAllocEx: 지정된 프로세스의 가상공간 내에서 메모리 영역의 상태를 예약, 커밋, 변경하는 함수
    // 반환값: 할당된 페이지 영역의 기본 주소
    const LPVOID lpPathAddress = VirtualAllocEx(hProcess, nullptr, lstrlenA(lpFullDLLPath) + 1, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (lpPathAddress == nullptr)
    {
        printf("An error is occured when trying to allocate memory in the target process.\n");
        return -1;
    }

    // 할당된 주소 확인
    printf("Memory allocate address: 0x%X\n", (UINT)(uintptr_t)lpPathAddress);

    // WriteProcessMemory: 지정된 프로세스의 데이터 영역에 데이터를 write
    // 반환값: write 실패 시 0
    const DWORD dwWriteResult = WriteProcessMemory(hProcess, lpPathAddress, lpFullDLLPath, lstrlenA(lpFullDLLPath) + 1, nullptr);
    if (dwWriteResult == 0)
    {
        printf("An error is occured when trying to write the DLL path in the target process.\n");
        return -1;
    }

    printf("DLL path writen successfully.\n");

    // GetModuleHandleA: 지정된 모듈의 handle을 받아오는 함수
    const HMODULE hModule = GetModuleHandleA("kernel32.dll");
    if (hModule == NULL) return -1; 

    // GetProcAddress: 지정된 DLL 내 지정된 함수의 주소를 반환하는 함수 
    // kernel32.dll의 LoadLibraryA 주소를 반환
    const FARPROC lpFunctionAddress = GetProcAddress(hModule, "LoadLibraryA");
    if (lpFunctionAddress == nullptr)
    {
        printf("An error is occured when trying to get \"LoadLibraryA\" address.\n");
        return -1;
    }

    // LoadLibraryA 함수 주소 확인
    printf("LoadLibraryA address at 0x%X\n", (UINT)(uintptr_t)lpFunctionAddress);

    // CreateRemoteThread: 다른 프로세스의 가상 주소 공간에서 실행되는 thread를 만드는 함수
    const HANDLE hThreadCreationResult = CreateRemoteThread(hProcess, nullptr, 0, (LPTHREAD_START_ROUTINE)lpFunctionAddress, lpPathAddress, 0, nullptr);
    if (hThreadCreationResult == INVALID_HANDLE_VALUE)
    {
        printf("An error is occured when trying to create the thread in the target process.\n");
        return -1;
    }

    printf("DLL Injected !\n");

    return 0;
}