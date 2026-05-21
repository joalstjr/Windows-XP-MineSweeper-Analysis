# 💣 Windows-XP-MineSweeper-Analysis & DLL Injection

> **Windows 기본 게임인 지뢰찾기(winmine.exe)를 리버스 엔지니어링하고, DLL 인젝션 기법을 통해 메모리 조작 및 치트 기능을 구현한 프로젝트입니다.**

## 📖 Project Overview
이 프로젝트는 보안 매커니즘이 적용되지 않은 32비트 애플리케이션의 메모리 구조와 실행 흐름을 분석하는 것을 목표로 합니다. 정적/동적 분석을 통해 얻은 오프셋을 바탕으로, 타깃 프로세스에 DLL을 주입(Injection)하여 GDI 기반 ESP, 실시간 메모리 변조, 내부 함수 강제 호출 등의 공격 기법을 실증합니다.

## 🛠️ Environment & Tools
| Category | Name | Version / Details |
| :---: | :--- | :--- |
| **OS** | Windows 11 (64-bit) | 타깃 프로그램은 32-bit (x86) |
| **Target** | `winmine.exe` | Windows XP Minesweeper |
| **Memory Scan** | Cheat Engine | 7.6 |
| **Dynamic Analysis**| OllyDbg | 1.10 |
| **Static Analysis** | Ghidra | 12.0.4 |
| **PE Analysis** | PEView | - |
| **Development** | Visual Studio 2022 | C/C++ (Injector & Hack DLL) |

---

## 🔍 Reverse Engineering Analysis
*기본 Image Base 주소는 `0x01000000`을 기준으로 작성되었습니다.*

### 1. Entry Point
| Description | Offset (Address) |
| :--- | :---: |
| **OEP (Original Entry Point)** | `winmine.exe+3E21` |

### 2. Global Variables (Memory Map)
| Description | Offset (Address) | Type / Size |
| :--- | :---: | :---: |
| **Time Value** (현재 진행 시간) | `winmine.exe+579C` | `DWORD` |
| **Flag Value** (꽂은 깃발 수) | `winmine.exe+5194` | `DWORD` |
| **Board Bound** (맵 데이터 시작점)| `winmine.exe+5341` | `BYTE Array` |
| **Board Height** (맵 세로 크기) | `winmine.exe+56A8` | `BYTE` |
| **Board Width** (맵 가로 크기) | `winmine.exe+56AC` | `BYTE` |
| **Mine Count** (총 지뢰 개수) | `winmine.exe+56A4` | `DWORD` |

### 3. Core & Cheat-Related Functions
*게임의 핵심 플레이 루프, 메모리 변조 및 치트 기능 구현에 직접 활용되거나 분석된 핵심 함수 목록입니다.*

| Function Name | Offset (Address) | Action (Description) |
| :--- | :---: | :--- |
| **WinMain** | `winmine.exe+21F0` | 프로그램 메인 엔트리 포인트 |
| **MainWndProc** | `winmine.exe+1BC9` | 메인 윈도우 프로시저 (마우스/키보드 이벤트 및 메시지 처리) |
| **setBoardFunc** | `winmine.exe+367A` | 지뢰 맵 생성 및 데이터 배열 초기화 루틴 |
| **ProcessTileClick** | `winmine.exe+37E1` | 타일(셀) 클릭 시 내부 로직 처리 (지뢰 판별, 빈칸 확장 등) |
| **OnFaceBtnClick** | `winmine.exe+140C` | 상단 스마일 버튼 클릭 이벤트 처리 (게임 리셋) |
| **setSmileImg** | `winmine.exe+28D9` | 스마일 아이콘 상태 변경 (웃음, 놀람, 선글라스, 사망 이미지) |
| **timereset** | `winmine.exe+346A` | 게임 타이머 변수 값을 0으로 초기화 |
| **winCheck** | `winmine.exe+347C` | 게임 승리 조건(지뢰를 제외한 모든 칸 오픈) 충족 여부 판정 |
| **ShowGameOverBoard** | `winmine.exe+2F80` | 지뢰 폭발(게임 오버) 시 맵 전체의 지뢰 위치를 강제로 노출 |
| **setGame** | `winmine.exe+3F90` | 난이도 및 사용자 정의 설정에 따른 게임 초기화 세팅 |
| **setTimer** | `winmine.exe+384F` | 타이머 값 갱신 및 UI 출력 제어 |
| **setMineCount** | `winmine.exe+2BC2` | 남은 지뢰(깃발) 개수 계산 및 UI 업데이트 |

### 4. Etc Functions
*메뉴 바 UI 제어, 신기록 기록, 에러 핸들링 및 도움말 래퍼 등 부가 기능을 수행하는 함수 목록입니다.*

| Function Name | Offset (Address) | Action (Description) |
| :--- | :---: | :--- |
| **registwinner** | `winmine.exe+1B81` | 최고 기록 달성 시 사용자 이름 입력 및 레지스트리 저장 |
| **showwinnerboard** | `winmine.exe+1BAA` | 최고 기록(Best Times) 순위표 윈도우 출력 |
| **setAllMenu** | `winmine.exe+1516` | 전체 상단 메뉴(초급, 중급, 고급 등) UI 상태 초기화 |
| **setEachMenu** | `winmine.exe+3CC4` | 난이도 변경에 따른 개별 메뉴 체크 상태 활성화/비활성화 |
| **ShowErrorMessage** | `winmine.exe+3950` | 게임 내 예외 상황 발생 시 에러 메시지 팝업 출력 |
| **ShowHtmlHelpWrapper** | `winmine.exe+4062` | 게임 도움말(F1) 창 호출 래퍼 함수 |
| **GetHtmlHelpControlPath** | `winmine.exe+40FB` | 도움말(CHM/HTML Help) 파일의 시스템 경로 탐색 |
| **Stub1** | `winmine.exe+4006` | 내부 더미 또는 컴파일러 스텁(Stub) 함수 |

---

## 🚀 Features (Cheat DLL)
본 프로젝트에서 구현한 `Hack.dll`은 타깃 프로세스에 주입된 후 콘솔 UI를 제공하며, 백그라운드 스레드를 통해 다음과 같은 기능을 수행합니다.

- [x] **Toggle ESP (Wallhack):** GDI `Rectangle` API를 후킹하여 메모리 상의 지뢰(`0x8F`) 위치 좌표를 계산, 타깃 윈도우 위에 노란색 테두리를 강제로 렌더링합니다.
- [x] **Auto-Reveal Mines:** `winmine.exe+5361`부터 시작하는 맵 데이터를 순회하며 지뢰 플래그 값을 노출 상태(`0x8A`)로 덮어써 화면에 가시화합니다.
- [x] **Force Win:** 리버싱으로 찾아낸 `winCheck` 함수 포인터에 직접 접근하여, 지뢰를 찾지 않은 상태에서도 강제로 게임 클리어 루틴을 실행시킵니다.

## 💻 How to Use
1. `winmine.exe` (지뢰찾기)를 실행합니다.
2. 터미널(cmd)을 관리자 권한으로 열고 다음과 같이 인젝터를 실행합니다.
   ```cmd
   inject.exe Hack.dll winmine.exe
