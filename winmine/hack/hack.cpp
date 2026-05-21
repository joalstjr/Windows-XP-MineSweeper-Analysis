#include <windows.h>
#include <iostream>
#include <cstdio>

bool g_bExit = false;   
bool g_bEsp = false;    
bool g_bReveal = false; 

DWORD WINAPI BackgroundThread(LPVOID lpParam) {
    HWND hWnd = FindWindowA("MineSweeper", NULL);
    if (!hWnd) return 0; 

    HDC hDc = GetDC(hWnd);
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 0)); 
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hDc, hBrush); 

    const uintptr_t START_ADDR = 0x1005361;

    while (!g_bExit) {
        if (!IsWindow(hWnd)) {
            g_bExit = true;
            break;
        }

        int board_h = *(BYTE*)0x10056A8;
        int board_w = *(BYTE*)0x10056AC;

        bool bNeedRedraw = false; 

        for (int height = 0; height < board_h; height++) {
            int top = 54 + (height * 16);

            uintptr_t row_addr = START_ADDR + (height * 0x20);

            for (int width = 0; width < board_w; width++) {
                BYTE* pValue = (BYTE*)(row_addr + width);
                BYTE value = *pValue; 

                if (g_bReveal && value == 0x8f) {
                    *pValue = 0x8a;     
                    value = 0x8a;           
                    bNeedRedraw = true; 
                }
                else if (!g_bReveal && value == 0x8a) {
                    *pValue = 0x8f;     
                    value = 0x8f;      
                    bNeedRedraw = true;
                }

                if (g_bEsp && (value == 0x8f || value == 0x8a)) {
                    int left = 11 + (width * 16);
                    Rectangle(hDc, left, top, left + 17, top + 17);
                }
            }
        }

        if (bNeedRedraw) {
            InvalidateRect(hWnd, NULL, TRUE); 
        }

        Sleep(50);
    }

    SelectObject(hDc, hOldBrush);
    DeleteObject(hBrush);
    ReleaseDC(hWnd, hDc);
    return 0;
}

DWORD WINAPI MainThread(LPVOID lpParam) {
    // 1. 타겟 프로세스(지뢰찾기) 내부에 콘솔(cmd) 창을 띄우고 제목을 설정
    AllocConsole();
    SetConsoleTitleA("MineSweeper Cheat Console");

    // 2. 새로 띄운 콘솔 창을 C언어의 표준 입출력(stdout, stdin)과 강제로 연결
    FILE* fpOut;
    FILE* fpIn;
    freopen_s(&fpOut, "CONOUT$", "w", stdout);
    freopen_s(&fpOut, "CONOUT$", "w", stderr);
    freopen_s(&fpIn, "CONIN$", "r", stdin);

    std::ios::sync_with_stdio(true);
    std::cout.clear();
    std::cin.clear();

    // 메뉴 출력 (3번 강제 승리 추가, 4번 종료로 변경)
    std::cout << "========== MINESWEEPER CHEAT ==========\n";
    std::cout << "1. Toggle ESP (ON/OFF)\n";
    std::cout << "2. Toggle Auto-Reveal Mines (ON/OFF)\n";
    std::cout << "3. Force Win\n";
    std::cout << "4. Exit & Unload DLL\n";
    std::cout << "=======================================\n\n";

    HANDLE hBgThread = CreateThread(NULL, 0, BackgroundThread, NULL, 0, NULL);

    int choice = 0;
    while (!g_bExit) {
        std::cout << "Input > ";

        if (!(std::cin >> choice)) {
            std::cin.clear();
            std::cin.ignore(10000, '\n');
            Sleep(100);
            continue;
        }

        if (choice == 1) {
            g_bEsp = !g_bEsp;
            std::cout << "[+] ESP is now " << (g_bEsp ? "ON" : "OFF") << "\n";
            if (!g_bEsp) {
                HWND hWnd = FindWindowA("MineSweeper", NULL);
                if (hWnd) InvalidateRect(hWnd, NULL, TRUE);
            }
        }
        else if (choice == 2) {
            g_bReveal = !g_bReveal;
            std::cout << "[+] Auto-Reveal is now " << (g_bReveal ? "ON" : "OFF") << "\n";
        }
        else if (choice == 3) {
            uintptr_t winCheckAddr = 0x0100347c; 

            typedef void(__cdecl* t_winCheck)(int);
            t_winCheck pWinCheck = (t_winCheck)winCheckAddr;

            std::cout << "[+] Calling winCheck(1)...\n";
            pWinCheck(1);
            std::cout << "[+] You Win!\n";
        }
        else if (choice == 4) {
            std::cout << "[+] Exiting...\n";
            g_bExit = true;
            break;
        }
        else {
            std::cout << "[-] Invalid Input. Please enter 1, 2, 3, or 4.\n";
        }
    }

    if (hBgThread) {
        WaitForSingleObject(hBgThread, INFINITE);
        CloseHandle(hBgThread);
    }

    if (fpOut) fclose(fpOut);
    if (fpIn) fclose(fpIn);
    FreeConsole();

    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
         HANDLE hThread = CreateThread(NULL, 0, MainThread, hModule, 0, NULL);
        if (hThread) CloseHandle(hThread); 
    }
    return TRUE;
}