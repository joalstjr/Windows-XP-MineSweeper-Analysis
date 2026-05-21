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

### 3. Core Functions
| Description | Offset (Address) | Action |
| :--- | :---: | :--- |
| **setGame** | `winmine.exe+3F90` | 게임 초기화 및 맵 세팅 |
| **setTimer** | `winmine.exe+384F` | 타이머 값 갱신 및 UI 출력 |
| **setMineCount** | `winmine.exe+2BC2` | 남은 지뢰(깃발) 개수 갱신 |
| **winCheck** | `winmine.exe+347C` | 게임 승리 조건 판정 (`arg=1` 시 승리) |

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
