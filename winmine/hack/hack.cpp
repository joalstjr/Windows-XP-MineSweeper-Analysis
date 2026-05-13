#include <windows.h>
//injector\release\injector.exe dll\release\dll.dll winmine.exe
DWORD WINAPI winmineCheat(LPVOID lpParam) {
    HWND hWnd = FindWindowA("MineSweeper", NULL);
    if (!hWnd) {
        FreeLibraryAndExitThread((HMODULE)lpParam, 0);
        return 0;
    }

    HDC hDc = GetDC(hWnd);
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 0));
    // GDI 누수 방지: 새 브러시를 적용하면서 원래 있던 브러시의 핸들을 저장해둠
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hDc, hBrush);

    // [최적화 1] 어차피 고정된 주소이므로 매번 뺄셈 연산할 필요 없이 상수로 박음
    const uintptr_t START_ADDR = 0x1005361;

    // [최적화 2] GetAsyncKeyState는 0x8000(최상위 비트)와 AND 연산을 해야 정확한 눌림 판정이 됨
    while ((GetAsyncKeyState(VK_END) & 0x8000) == 0) {

        // [안정성] END 키를 누르기 전에 지뢰찾기 창을 꺼버렸을 때, 무한 루프에 빠지는 것을 방지
        if (!IsWindow(hWnd)) break;

        // 난이도 변경에 대응하기 위해 루프 안에서 보드 크기를 계속 읽어옴
        int board_h = *(BYTE*)0x10056A8;
        int board_w = *(BYTE*)0x10056AC;

        for (int height = 0; height < board_h; height++) {
            // [최적화 3] 루프 안에서 매번 계산할 필요 없는 'Y좌표'와 '현재 행(Row)의 메모리 주소'를 바깥으로 뺌
            int top = 54 + (height * 16);
            uintptr_t row_addr = START_ADDR + (height * 0x20);

            for (int width = 0; width < board_w; width++) {
                // 더하기 연산만으로 깔끔하게 주소 접근
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