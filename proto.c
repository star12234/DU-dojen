#include <windows.h>
#include <sapi.h>       // Microsoft Speech API (TTS)
#include <tchar.h>      // 문자열 관련
#include <stdio.h>
#include <string.h>

// ------------------------------
// 음성 합성(TTS) 기능
// ------------------------------
void SpeakText(const wchar_t *text) {
    ISpVoice *pVoice = NULL;

    // COM 라이브러리 초기화
    if (FAILED(CoInitialize(NULL))) return;

    // SAPI 음성 엔진 인스턴스 생성
    if (SUCCEEDED(CoCreateInstance(&CLSID_SpVoice, NULL, CLSCTX_ALL, &IID_ISpVoice, (void **)&pVoice))) {
        pVoice->lpVtbl->Speak(pVoice, text, SPF_IS_NOT_XML, NULL); // 음성 출력
        pVoice->lpVtbl->Release(pVoice); // 메모리 해제
    }

    // COM 라이브러리 종료
    CoUninitialize();
}

// ------------------------------
// 현재 활성 창의 제목 읽기
// ------------------------------
void ReadActiveWindowTitle() {
    wchar_t windowTitle[256]; // 창 제목 저장 배열

    HWND hwnd = GetForegroundWindow(); // 현재 활성 창 핸들 가져오기
    if (hwnd) {
        GetWindowTextW(hwnd, windowTitle, sizeof(windowTitle) / sizeof(wchar_t)); // 창 제목 가져오기
        if (wcslen(windowTitle) > 0) {
            wprintf(L"창 제목: %s\n", windowTitle);
            SpeakText(windowTitle); // 음성 출력
        }
    }
}

// ------------------------------
// 키보드 입력 감지 후 실시간 읽기
// ------------------------------
LRESULT CALLBACK KeyboardHookProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode == HC_ACTION && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN)) {
        KBDLLHOOKSTRUCT *kbdStruct = (KBDLLHOOKSTRUCT *)lParam;
        wchar_t key[32]; // 키 저장 배열

        // 키 이름 변환 (가독성을 위해 변환)
        if (GetKeyNameTextW(kbdStruct->scanCode << 16, key, sizeof(key) / sizeof(wchar_t)) > 0) {
            wprintf(L"입력 키: %s\n", key);
            SpeakText(key); // 음성 출력
        }
    }
    return CallNextHookEx(NULL, nCode, wParam, lParam);
}

// ------------------------------
// 마우스 포인터가 위치한 창 제목 읽기
// ------------------------------
void ReadTextUnderMouse() {
    POINT p;
    GetCursorPos(&p); // 현재 마우스 위치 가져오기
    HWND hwnd = WindowFromPoint(p); // 해당 위치의 창 핸들 가져오기

    if (hwnd) {
        wchar_t windowText[256];
        GetWindowTextW(hwnd, windowText, sizeof(windowText) / sizeof(wchar_t));
        if (wcslen(windowText) > 0) {
            wprintf(L"마우스 위치 창: %s\n", windowText);
            SpeakText(windowText);
        }
    }
}

// ------------------------------
// OCR(광학 문자 인식) 기능 (추가 필요)
// ------------------------------
void PerformOCR() {
    // 실제 OCR 라이브러리(Tesseract 등)를 연동하여 화면에 보이는 텍스트 인식 가능
    SpeakText(L"OCR 기능이 곧 추가될 예정입니다.");
}

// ------------------------------
// 프로그램 실행 (메인 루프)
// ------------------------------
int main() {
    // 키보드 입력 감지를 위한 훅 설정
    HHOOK keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, GetModuleHandle(NULL), 0);

    // 무한 루프 실행 (백그라운드 감지)
    while (1) {
        ReadActiveWindowTitle(); // 현재 창 제목 읽기
        ReadTextUnderMouse(); // 마우스 위치 텍스트 읽기
        Sleep(2000); // 2초마다 실행
    }

    // 키보드 훅 해제
    UnhookWindowsHookEx(keyboardHook);
    return 0;
}

