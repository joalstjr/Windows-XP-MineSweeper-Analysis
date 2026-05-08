#include <windows.h>
DWORD WINAPI winmineCheat(LPVOID lpParam) {
    BYTE winmine_exe = 0x1000000;
    HWND hWnd = FindWindowA("MineSweeper", NULL);
    if (!hWnd) {
        FreeLibraryAndExitThread((HMODULE)lpParam, 0);
        return 0;
    }

    HDC hDc = GetDC(hWnd);
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 0));
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hDc, hBrush);

    const uintptr_t START_ADDR = winmine_exe+0x5361;

    while ((GetAsyncKeyState(VK_END) & 0x8000) == 0) {
        if (!IsWindow(hWnd)) break;

        // board의 가로 세로 크기를 메모리에서 읽음
        BYTE board_h = *(BYTE*)(winmine_exe +0x56A8);
        BYTE board_w = *(BYTE*)(winmine_exe +0x56AC);

        for (BYTE height = 0; height < board_h; height++) {
            int top = 54 + (height * 16);
            uintptr_t row_addr = START_ADDR + (height * 0x20);

            for (BYTE width = 0; width < board_w; width++) {
                BYTE value = *(BYTE*)(row_addr + width);

                if (value == 0x8f) {
                    int left = 11 + (width * 16);
                    Rectangle(hDc, left, top, left + 17, top + 17);
                }
            }
        }
        Sleep(50);
    }

    SelectObject(hDc, hOldBrush);
    DeleteObject(hBrush);
    ReleaseDC(hWnd, hDc);
    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(NULL, 0, winmineCheat, hModule, 0, NULL);
        if (hThread) CloseHandle(hThread);
    }
    return TRUE;
}