#include <windows.h>
#include <iostream>
#include <cstdio>

// =======================================================================================
// [전역 변수] 두 개의 스레드(메인 제어, 백그라운드 작업)가 상태를 공유하기 위해 사용합니다.
// =======================================================================================
bool g_bExit = false;   // 프로그램 종료 플래그 (true가 되면 모든 루프를 탈출하고 DLL을 언로드함)
bool g_bEsp = false;    // ESP(지뢰 위치 노란색 네모 표시) 기능 ON/OFF 상태
bool g_bReveal = false; // 자동 지뢰 공개 기능 ON/OFF 상태

// =======================================================================================
// [스레드 1] 백그라운드 메모리 조작 및 ESP(GDI 그리기) 스레드
// 지뢰찾기 프로세스 내에서 무한 루프를 돌며 메모리를 읽고/쓰고, 화면에 그림을 그립니다.
// =======================================================================================
DWORD WINAPI BackgroundThread(LPVOID lpParam) {
    // 1. 지뢰찾기 창 핸들(ID)을 찾아옵니다. (화면에 그림을 그리거나 창 상태를 갱신하기 위함)
    HWND hWnd = FindWindowA("MineSweeper", NULL);
    if (!hWnd) return 0; // 창을 못 찾으면 스레드 종료

    // 2. GDI(그래픽 디바이스 인터페이스) 세팅
    // HDC(Device Context)는 도화지, HBRUSH는 붓이라고 생각하면 됩니다.
    HDC hDc = GetDC(hWnd);
    HBRUSH hBrush = CreateSolidBrush(RGB(255, 255, 0)); // 노란색 붓 생성
    HBRUSH hOldBrush = (HBRUSH)SelectObject(hDc, hBrush); // 기존 붓을 저장하고 노란 붓을 쥠

    // 지뢰찾기 보드의 첫 번째 타일(좌측 상단)의 하드코딩된 메모리 시작 주소
    const uintptr_t START_ADDR = 0x1005361;

    // g_bExit가 true가 될 때까지 무한 반복 (Sleep(50)을 통해 0.05초마다 실행)
    while (!g_bExit) {
        // 사용자가 지뢰찾기 창을 X 버튼으로 꺼버렸을 경우, 무한루프를 탈출하여 에러 방지
        if (!IsWindow(hWnd)) {
            g_bExit = true;
            break;
        }

        // 난이도(초급/중급/고급)에 따라 맵 크기가 변하므로, 현재 보드의 높이와 너비 메모리를 계속 읽어옴
        int board_h = *(BYTE*)0x10056A8;
        int board_w = *(BYTE*)0x10056AC;

        bool bNeedRedraw = false; // 화면 갱신(새로고침)이 필요한지 체크하는 플래그

        // 2차원 배열 형태인 지뢰 보드를 순회 (Y축 -> X축)
        for (int height = 0; height < board_h; height++) {
            // Y좌표 픽셀 계산: 기본 상단 여백(54픽셀) + (현재 행 * 16픽셀)
            int top = 54 + (height * 16);

            // 현재 행(Row)의 메모리 시작 주소: 보드의 한 줄은 메모리 상에서 0x20(32바이트) 단위로 할당됨
            uintptr_t row_addr = START_ADDR + (height * 0x20);

            for (int width = 0; width < board_w; width++) {
                // 특정 타일의 메모리 주소(row_addr + width)를 포인터로 변환하여 값을 읽음
                BYTE* pValue = (BYTE*)(row_addr + width);
                BYTE value = *pValue; // 실제 타일의 상태값 (예: 빈칸, 지뢰, 숫자 등)

                // ---------------------------------------------------------
                // [기능 1: 지뢰 자동 공개 로직]
                // g_bReveal이 켜져있고, 해당 칸이 '숨겨진 지뢰(0x8f)'라면
                // ---------------------------------------------------------
                if (g_bReveal && value == 0x8f) {
                    *pValue = 0x8a;     // 메모리에 '공개된 지뢰(0x8a)' 값을 덮어써서 조작 (Memory Patch)
                    value = 0x8a;       // 방금 값을 바꿨으므로, 바로 아래 ESP 로직을 위해 현재 변수값도 0x8a로 동기화
                    bNeedRedraw = true; // 메모리가 변조되었으니 화면을 새로고침 해야 함을 마킹
                }

                // ---------------------------------------------------------
                // [기능 2: ESP (핵 사용자에게만 보이는 위치 표시) 로직]
                // g_bEsp가 켜져있고, 해당 칸이 지뢰(숨겨진 지뢰 0x8f 또는 공개된 지뢰 0x8a)라면
                // ---------------------------------------------------------
                if (g_bEsp && (value == 0x8f || value == 0x8a)) {
                    // X좌표 픽셀 계산: 기본 좌측 여백(11픽셀) + (현재 열 * 16픽셀)
                    int left = 11 + (width * 16);
                    // 계산된 픽셀 위치에 17x17 크기의 노란색 사각형을 그림
                    Rectangle(hDc, left, top, left + 17, top + 17);
                }
            }
        }

        // 지뢰를 덮어쓴 순간(메모리가 변조된 순간)에만 딱 한 번 화면 새로고침
        // 무조건 새로고침하면 화면이 심하게 깜빡임(Flickering)
        if (bNeedRedraw) {
            InvalidateRect(hWnd, NULL, TRUE); // 지뢰찾기의 메인 UI 스레드에게 WM_PAINT 메시지를 보내 화면을 다시 그리게 함
        }

        Sleep(50); // CPU 점유율이 100%로 치솟는 것을 방지하기 위한 대기 (50ms)
    }

    // 루프 탈출(프로그램 종료) 시, 할당했던 자원(붓, 도화지)을 운영체제에 반환 (메모리 누수 방지)
    SelectObject(hDc, hOldBrush);
    DeleteObject(hBrush);
    ReleaseDC(hWnd, hDc);
    return 0;
}

// =======================================================================================
// [스레드 2] 콘솔 메뉴 입력 및 제어 스레드
// 사용자로부터 1, 2, 3 입력을 받아 전역 상태 변수를 제어하는 역할입니다.
// =======================================================================================
DWORD WINAPI MainThread(LPVOID lpParam) {
    // 1. 타겟 프로세스(지뢰찾기) 내부에 콘솔(cmd) 창을 띄우고 제목을 설정
    AllocConsole();
    SetConsoleTitleA("MineSweeper Cheat Console");

    // 2. 새로 띄운 콘솔 창을 C언어의 표준 입출력(stdout, stdin)과 강제로 연결 (스트림 바인딩)
    FILE* fpOut;
    FILE* fpIn;
    freopen_s(&fpOut, "CONOUT$", "w", stdout);
    freopen_s(&fpOut, "CONOUT$", "w", stderr);
    freopen_s(&fpIn, "CONIN$", "r", stdin);

    // 3. C++ 의 iostream(std::cout, std::cin) 버퍼를 초기화하고 C 입출력과 동기화
    std::ios::sync_with_stdio(true);
    std::cout.clear();
    std::cin.clear();

    // 메뉴 출력
    std::cout << "========== MINESWEEPER CHEAT ==========\n";
    std::cout << "1. Toggle ESP (ON/OFF)\n";
    std::cout << "2. Toggle Auto-Reveal Mines (ON/OFF)\n";
    std::cout << "3. Exit & Unload DLL\n";
    std::cout << "=======================================\n\n";

    // 입력을 대기하기 전에, 뒤에서 지뢰를 스캔하고 그릴 백그라운드 스레드를 생성 및 실행
    HANDLE hBgThread = CreateThread(NULL, 0, BackgroundThread, NULL, 0, NULL);

    int choice = 0;
    while (!g_bExit) {
        std::cout << "Input > ";

        // [안정성 코드] 사용자가 숫자가 아닌 문자를 입력했을 때 std::cin이 고장나서 무한루프 도는 현상 방지
        if (!(std::cin >> choice)) {
            std::cin.clear();             // 에러 플래그 초기화
            std::cin.ignore(10000, '\n'); // 버퍼에 남은 쓰레기값 제거
            Sleep(100);
            continue;
        }

        // 1번 입력: ESP 토글
        if (choice == 1) {
            g_bEsp = !g_bEsp; // true <-> false 반전
            std::cout << "[+] ESP is now " << (g_bEsp ? "ON" : "OFF") << "\n";

            // ESP를 껐을 때 기존에 화면에 그려진 노란색 잔상을 지우기 위해 화면 강제 갱신
            if (!g_bEsp) {
                HWND hWnd = FindWindowA("MineSweeper", NULL);
                if (hWnd) InvalidateRect(hWnd, NULL, TRUE);
            }
        }
        // 2번 입력: 자동 지뢰 공개 토글
        else if (choice == 2) {
            g_bReveal = !g_bReveal; // true <-> false 반전
            std::cout << "[+] Auto-Reveal is now " << (g_bReveal ? "ON" : "OFF") << "\n";
        }
        // 3번 입력: 해킹 종료 및 DLL 언로드
        else if (choice == 3) {
            std::cout << "[+] Exiting...\n";
            g_bExit = true; // 전역 플래그를 true로 바꿔 BackgroundThread의 루프를 종료시킴
            break;          // MainThread의 while 루프 탈출
        }
        else {
            std::cout << "[-] Invalid Input. Please enter 1, 2, or 3.\n";
        }
    }

    // -----------------------------------------------------------------------------------
    // [종료 시퀀스] 프로그램이 크래시 나지 않고 깔끔하게 빠져나가기 위한 정리 작업
    // -----------------------------------------------------------------------------------

    // 메인 스레드가 바로 죽지 않고, 백그라운드 스레드가 완전히 루프를 탈출하고 종료될 때까지 대기
    if (hBgThread) {
        WaitForSingleObject(hBgThread, INFINITE);
        CloseHandle(hBgThread);
    }

    // 할당했던 콘솔 창 및 스트림 반환
    if (fpOut) fclose(fpOut);
    if (fpIn) fclose(fpIn);
    FreeConsole();

    // 지뢰찾기 메모리에서 이 DLL을 안전하게 해제하고 메인 스레드를 종료
    FreeLibraryAndExitThread((HMODULE)lpParam, 0);
    return 0;
}

// =======================================================================================
// [DLL 진입점] 인젝터에 의해 DLL이 지뢰찾기 프로세스에 삽입될 때 최초로 실행되는 함수
// =======================================================================================
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    // DLL이 프로세스에 붙었을 때 (인젝션 성공 시)
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        // 불필요한 스레드 호출 알림을 꺼서 성능 최적화
        DisableThreadLibraryCalls(hModule);

        // 콘솔 창을 띄우고 입력을 받을 메인 스레드를 백그라운드에서 생성
        // hModule을 파라미터로 넘겨서 나중에 FreeLibraryAndExitThread 시 자기 자신을 메모리에서 해제할 수 있게 함
        HANDLE hThread = CreateThread(NULL, 0, MainThread, hModule, 0, NULL);
        if (hThread) CloseHandle(hThread); // 스레드 핸들(껍데기)만 닫고 스레드 자체는 계속 실행되게 둠
    }
    return TRUE;
}
