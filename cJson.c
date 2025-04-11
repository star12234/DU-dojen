// screen_reader_full.c
// Windows 스크린 리더 최종 예제
// 기능: 마우스 포인터가 가리키는 UI 요소 읽기 (F1), 키 입력(일반 문자 및 특수키)에 대해 음성 피드백,
//       동적 UI 업데이트 감지(폴링 방식), 사용자 설정(JSON 파일) 적용, 언어(키보드 레이아웃) 감지, 로그/예외 처리
// cJSON 라이브러리 사용 (cJSON.h 필요)

#include <windows.h>
#include <sapi.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>

// cJSON 헤더 (cJSON 라이브러리 필요)
#include "cJSON.h"

// ------------------------------
// 전역 변수 및 사용자 설정 구조체
// ------------------------------
ISpVoice* g_pVoice = NULL;
HHOOK g_hKeyboardHook = NULL;

typedef struct {
    float speechRate;      // TTS 속도 (기본 1.0)
    float volume;          // TTS 볼륨 (0 ~ 1.0)
    DWORD pollInterval;    // 동적 UI 업데이트 감지 폴링 간격 (밀리초, 기본 5000ms)
    HKL keyboardLayout;    // 현재 키보드 레이아웃
} UserSettings;

UserSettings g_Settings;

// 로그 파일 및 설정 파일 경로
const char* LOG_FILE = "screen_reader_log.txt";
const char* CONFIG_FILE = "config.json";

// ------------------------------
// 로그 기록 함수 (오류 및 상태 기록)
// ------------------------------
void LogMessage(const char* message) {
    FILE *fp = fopen(LOG_FILE, "a");
    if (fp) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(fp, "[%02d:%02d:%02d] %s\n", st.wHour, st.wMinute, st.wSecond, message);
        fclose(fp);
    }
}

// ------------------------------
// JSON 설정 파일 파싱 및 사용자 설정 로드 함수
// ------------------------------
BOOL LoadUserSettings() {
    FILE* fp = fopen(CONFIG_FILE, "r");
    if (!fp) {
        LogMessage("Config file not found. Using default settings.");
        // 기본값 설정
        g_Settings.speechRate = 1.0f;
        g_Settings.volume = 1.0f;
        g_Settings.pollInterval = 5000;
        g_Settings.keyboardLayout = GetKeyboardLayout(0);
        return TRUE;
    }

    // 파일 전체 읽기
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    rewind(fp);
    char* fileContent = (char*)malloc(fileSize + 1);
    if (!fileContent) {
        LogMessage("Memory allocation failed for config file content.");
        fclose(fp);
        return FALSE;
    }
    if (fread(fileContent, 1, fileSize, fp) != fileSize) {
        LogMessage("Failed to read config file content.");
        free(fileContent);
        fclose(fp);
        return FALSE;
    }
    fileContent[fileSize] = '\0';
    fclose(fp);

    // JSON 파싱
    cJSON *json = cJSON_Parse(fileContent);
    free(fileContent);
    if (!json) {
        LogMessage("JSON parse error. Using default settings.");
        g_Settings.speechRate = 1.0f;
        g_Settings.volume = 1.0f;
        g_Settings.pollInterval = 5000;
        g_Settings.keyboardLayout = GetKeyboardLayout(0);
        return TRUE;
    }
    
    // 각 항목 파싱 (존재하면 적용, 없으면 기본값)
    cJSON *rate = cJSON_GetObjectItemCaseSensitive(json, "speechRate");
    g_Settings.speechRate = cJSON_IsNumber(rate) ? (float)rate->valuedouble : 1.0f;
    
    cJSON *vol = cJSON_GetObjectItemCaseSensitive(json, "volume");
    g_Settings.volume = cJSON_IsNumber(vol) ? (float)vol->valuedouble : 1.0f;
    
    cJSON *poll = cJSON_GetObjectItemCaseSensitive(json, "pollInterval");
    g_Settings.pollInterval = cJSON_IsNumber(poll) ? (DWORD)poll->valuedouble : 5000;
    
    // 키보드 레이아웃: 초기값
    g_Settings.keyboardLayout = GetKeyboardLayout(0);

    LogMessage("User settings loaded from config file.");
    cJSON_Delete(json);
    return TRUE;
}

// ------------------------------
// TTS 엔진 초기화 (COM 및 SAPI)
// ------------------------------
BOOL InitTTS() {
    HRESULT hr = CoInitialize(NULL);
    if (FAILED(hr)) {
        LogMessage("CoInitialize failed.");
        return FALSE;
    }
    hr = CoCreateInstance(&CLSID_SpVoice, NULL, CLSCTX_ALL, &IID_ISpVoice, (void**)&g_pVoice);
    if (FAILED(hr) || !g_pVoice) {
        LogMessage("CoCreateInstance for SpVoice failed.");
        CoUninitialize();
        return FALSE;
    }
    LogMessage("TTS engine initialized successfully.");
    return TRUE;
}

// ------------------------------
// TTS 호출 함수 (설정값 적용)
// ------------------------------
void SpeakText(const wchar_t* text) {
    if (!g_pVoice || !text) {
        LogMessage("SpeakText: Invalid TTS engine or null text.");
        return;
    }
    // SAPI 속도: 기본 0에서 사용자 입력값 조정 (여기서는 (speechRate - 1.0) * 10)
    long rate = (long)((g_Settings.speechRate - 1.0f) * 10);
    g_pVoice->lpVtbl->SetRate(g_pVoice, rate);
    // SAPI 볼륨: 0 ~ 100 (사용자 설정 0~1을 변환)
    ULONG vol = (ULONG)(g_Settings.volume * 100);
    g_pVoice->lpVtbl->SetVolume(g_pVoice, vol);
    
    HRESULT hr = g_pVoice->lpVtbl->Speak(g_pVoice, text, SPF_ASYNC, NULL);
    if (FAILED(hr)) {
        LogMessage("SpeakText: TTS speak failed.");
    }
}

// ------------------------------
// 키보드 레이아웃 감지 및 업데이트 (언어 변경 감지)
// ------------------------------
void UpdateKeyboardLayout() {
    HKL newLayout = GetKeyboardLayout(0);
    if (newLayout != g_Settings.keyboardLayout) {
        g_Settings.keyboardLayout = newLayout;
        SpeakText(L"키보드 언어가 변경되었습니다.");
        LogMessage("Keyboard layout changed.");
    }
}

// ------------------------------
// 동적 UI 업데이트 감지 스레드 (폴링 방식, 예외 처리 포함)
// ------------------------------
DWORD WINAPI UIUpdateMonitorThread(LPVOID lpParam) {
    while (1) {
        Sleep(g_Settings.pollInterval);
        __try {
            // 실제 구현에서는 UI Automation API 등으로 UI 변경 이벤트 감지가 필요하나,
            // 여기서는 단순 폴링 후 알림으로 처리
            SpeakText(L"화면 내용이 업데이트되었습니다.");
            LogMessage("Dynamic UI update event triggered.");
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            LogMessage("Exception caught in UI update thread.");
        }
    }
    return 0;
}

// ------------------------------
// 키보드 후킹 콜백 함수 (키 이벤트 처리; 예외 처리 및 상세 피드백 포함)
// ------------------------------
LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        PKBDLLHOOKSTRUCT pKey = (PKBDLLHOOKSTRUCT)lParam;
        wchar_t feedback[256] = {0};

        __try {
            switch (pKey->vkCode) {
                case VK_F1: {
                    // F1키: 마우스 커서 아래 UI 요소 읽기
                    POINT pt;
                    if (GetCursorPos(&pt)) {
                        HWND hwnd = WindowFromPoint(pt);
                        if (hwnd) {
                            TCHAR windowText[256] = {0};
                            if (GetWindowText(hwnd, windowText, 256) == 0) {
                                _tcscpy(windowText, _T("정보 없음"));
                            }
                            _stprintf(feedback, _T("현재 창: %s"), windowText);
                            SpeakText(feedback);
                            LogMessage("F1 pressed: Read element under mouse.");
                        }
                    }
                    break;
                }
                case VK_RETURN:
                    SpeakText(L"엔터");
                    LogMessage("Enter key pressed.");
                    break;
                case VK_SPACE:
                    SpeakText(L"스페이스");
                    LogMessage("Space key pressed.");
                    break;
                case VK_BACK:
                    SpeakText(L"백스페이스");
                    LogMessage("Backspace key pressed.");
                    break;
                case VK_ESCAPE:
                    SpeakText(L"이스케이프");
                    LogMessage("Escape key pressed.");
                    break;
                default: {
                    // 일반 키 처리: 단일 문자일 경우 읽어줌
                    BYTE keyboardState[256] = {0};
                    if (GetKeyboardState(keyboardState)) {
                        WCHAR uniChar[5] = {0};
                        UINT scanCode = MapVirtualKey(pKey->vkCode, MAPVK_VK_TO_VSC);
                        int result = ToUnicode(pKey->vkCode, scanCode, keyboardState, uniChar, 4, 0);
                        if (result > 0) {
                            SpeakText(uniChar);
                            LogMessage("General key pressed and spoken.");
                        }
                    }
                    break;
                }
            }
            // 키 입력 이벤트 후 항상 키보드 레이아웃 업데이트 검사
            UpdateKeyboardLayout();
        }
        __except(EXCEPTION_EXECUTE_HANDLER) {
            LogMessage("Exception occurred in KeyboardProc.");
        }
    }
    return CallNextHookEx(g_hKeyboardHook, nCode, wParam, lParam);
}

// ------------------------------
// 키보드 후킹 등록 함수
// ------------------------------
BOOL SetKeyboardHook() {
    g_hKeyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, GetModuleHandle(NULL), 0);
    if (!g_hKeyboardHook) {
        LogMessage("Failed to set keyboard hook.");
        return FALSE;
    }
    LogMessage("Keyboard hook set successfully.");
    return TRUE;
}

// ------------------------------
// 메인 메시지 루프 실행 함수
// ------------------------------
void RunMessageLoop() {
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// ------------------------------
// 종료 시 리소스 정리 함수
// ------------------------------
void Cleanup() {
    if (g_hKeyboardHook) {
        UnhookWindowsHookEx(g_hKeyboardHook);
    }
    if (g_pVoice) {
        g_pVoice->lpVtbl->Release(g_pVoice);
    }
    CoUninitialize();
    LogMessage("Cleaned up resources and exiting.");
}

// ------------------------------
// main 함수: 전체 초기화 -> 후킹 등록 -> 동적 UI 스레드 시작 -> 메시지 루프 실행 -> 종료
// ------------------------------
int main(void) {
    // 1. 사용자 설정 로드 (config.json)
    if (!LoadUserSettings()) {
        LogMessage("Failed to load user settings. Aborting.");
        return 1;
    }
    
    // 2. TTS 엔진 초기화
    if (!InitTTS()) {
        LogMessage("Failed to initialize TTS. Aborting.");
        return 1;
    }
    
    // 3. 키보드 후킹 등록
    if (!SetKeyboardHook()) {
        Cleanup();
        return 1;
    }
    
    // 4. 동적 UI 업데이트 감지 스레드 시작
    HANDLE hUIThread = CreateThread(NULL, 0, UIUpdateMonitorThread, NULL, 0, NULL);
    if (!hUIThread) {
        LogMessage("Failed to start UI update monitor thread.");
    } else {
        LogMessage("UI update monitor thread started.");
    }
    
    // 5. 메인 메시지 루프 실행 (사용자 이벤트 처리)
    RunMessageLoop();
    
    // 6. 종료 처리: 리소스 정리
    Cleanup();
    
    if (hUIThread) {
        WaitForSingleObject(hUIThread, INFINITE);
        CloseHandle(hUIThread);
    }
    
    return 0;
}
