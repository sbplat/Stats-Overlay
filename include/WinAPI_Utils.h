/*
MIT License

Copyright (c) 2022 sbplat

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#pragma once

#include "File_Loader.h"

// #include <SDL2/SDL.h>
#include <spdlog/spdlog.h>

#include <string>
#include <regex>

#include <windows.h>


#ifndef WIN_API_UTILS_H
#define WIN_API_UTILS_H

namespace WAPIUtil {

    namespace F11Hook {

        const int MAX_LENGTH = 63;
        const std::regex lunarClientRegex("^Lunar Client \\(\\d.\\d.\\d-(.+)/(master)\\)[\\s\\S]*$"),
              minecraftRegex("^Minecraft \\d.\\d.\\d[\\s\\S]*$");

        bool fakeFullscreen;

        void fakeFullScreen(HWND hWnd) {
            int w = GetSystemMetrics(SM_CXSCREEN);
            int h = GetSystemMetrics(SM_CYSCREEN);

            if (GetWindowLongPtr(hWnd, GWL_STYLE) & WS_OVERLAPPEDWINDOW) {
                // Get into fake fullscreen
                spdlog::info("F11 key pressed. Putting Minecraft into fake fullscreen mode");
                SetWindowLongPtr(hWnd, GWL_STYLE, WS_VISIBLE);
                SetWindowPos(hWnd, NULL, 0, 0, w, h, SWP_FRAMECHANGED);

            } else {
                // Get out of fake fullscreen
                spdlog::info("F11 key pressed. Putting Minecraft into normal windowed mode");
                SetWindowLongPtr(hWnd, GWL_STYLE, WS_VISIBLE | WS_OVERLAPPEDWINDOW);
                SetWindowPos(hWnd, NULL, w / 4, h / 4, w / 2, h / 2, SWP_FRAMECHANGED);
            }
        }

        bool matchWindowTitle(std::string windowTitle) {
            if (std::regex_match(windowTitle, minecraftRegex)) {
                return true;
            }

            if (std::regex_match(windowTitle, lunarClientRegex)) {
                return true;
            }

            return false;
        }

        bool processF11Keypress() {
            if (!fakeFullscreen) {
                // Fake fullscreen is disabled
                return false;
            }

            HWND foregroundWindowHWND = GetForegroundWindow();
            char buffer[MAX_LENGTH + 1];
            GetWindowText(foregroundWindowHWND, buffer, MAX_LENGTH + 1);

            std::string activeWindowTitle(buffer, buffer + MAX_LENGTH);

            if (matchWindowTitle(activeWindowTitle)) {
                fakeFullScreen(foregroundWindowHWND);
                return true;

            } else {
                return false;
            }
        }

    } // F11Hook

    namespace Hotkey {

        bool oKeyPressed = false, shiftKeyPressed = false, ctrlKeyPressed = false;

        LRESULT CALLBACK LowLevelKeyboardProc(int nCode, WPARAM wParam, LPARAM lParam) {
            KBDLLHOOKSTRUCT *key = (KBDLLHOOKSTRUCT *)lParam;

            switch (wParam) {
                case WM_KEYDOWN: {
                        DWORD vkCode = key->vkCode;

                        if (vkCode == 0x4F) {
                            // o key pressed
                            oKeyPressed = true;
                            // std::cout << "o down" << std::endl;
                        }

                        if (vkCode == 0xA0 || vkCode == 0xA1) {
                            // Shift key pressed
                            shiftKeyPressed = true;
                            // std::cout << "shift down" << std::endl;
                        }

                        if (vkCode == 0xA2 || vkCode == 0xA3) {
                            // Ctrl key pressed
                            ctrlKeyPressed = true;
                            // std::cout << "ctrl down" << std::endl;
                        }

                        if (vkCode == 0x7A) {
                            // F11 key pressed
                            if (F11Hook::processF11Keypress()) {
                                // prevent F11 from being pressed
                                return 1;
                            }
                        }

                        break;
                    }

                case WM_KEYUP: {
                        DWORD vkCode = key->vkCode;

                        if (vkCode == 0x4F) {
                            // o key released
                            oKeyPressed = false;
                            // std::cout << "o up" << std::endl;
                        }

                        if (vkCode == 0xA0 || vkCode == 0xA1) {
                            // Shift key released
                            shiftKeyPressed = false;
                            // std::cout << "shift up" << std::endl;
                        }

                        if (vkCode == 0xA2 || vkCode == 0xA3) {
                            // Ctrl key released
                            ctrlKeyPressed = false;
                            // std::cout << "ctrl up" << std::endl;
                        }

                        /*
                        // only key up is registered
                        if (vkCode == 0x7A) {
                            // F11 key released
                        }
                        */

                        break;
                    }
            }

            return CallNextHookEx(NULL, nCode, wParam, lParam);
        }

        void keyboardShortcutLoop() {
            HINSTANCE hExe = GetModuleHandle(NULL);

            if (hExe) {
                spdlog::info("Starting keyboard shortcut loop...");
                // Hook onto Ctrl, Shift, O and F11 key presses
                HHOOK hKeyHook = SetWindowsHookEx(WH_KEYBOARD_LL, (HOOKPROC)LowLevelKeyboardProc, hExe, 0);

                MSG msg;

                while (running.load() && GetMessage(&msg, NULL, 0, 0) != 0) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                UnhookWindowsHookEx(hKeyHook);

            } else {
                spdlog::error("Could not start keyboard shortcut loop");
            }
        }

        bool shortcutKeysPressed() {
            // All three keys must be pressed simultaneously
            return oKeyPressed && shiftKeyPressed && ctrlKeyPressed;
        }

    } // Hotkey

} // WAPIUtil

#endif // WIN_API_UTILS_H
