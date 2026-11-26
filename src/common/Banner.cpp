/*
 * This file is part of the TrinityCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Banner.h"
#include "GitRevision.h"
#include "StringFormat.h"
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace
{
    // MiauCore banner (plain ASCII so it renders with any console code page)
    char const* const BannerLines[] =
    {
        "    /\\_/\\      __  __ _              ____",
        "   ( o.o )    |  \\/  (_) __ _ _   _ / ___|___  _ __ ___",
        "    > ^ <     | |\\/| | |/ _` | | | | |   / _ \\| '__/ _ \\",
        "   /     \\    | |  | | | (_| | |_| | |__| (_) | | |  __/",
        "  (_______)   |_|  |_|_|\\__,_|\\__,_|\\____\\___/|_|  \\___|",
    };

    // 256-color gradient: pink -> violet -> cyan
    int const GradientColors[] = { 213, 177, 141, 105, 75 };

    bool PrepareConsole(char const* applicationName)
    {
#ifdef _WIN32
        HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
        if (out == INVALID_HANDLE_VALUE || out == nullptr)
            return false;

        DWORD mode = 0;
        if (!GetConsoleMode(out, &mode))
            return false; // not a console (redirected to a file or a service)

        // ANSI / VT sequences (Windows 10+ conhost and Windows Terminal)
        if (!SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT))
            return false;

        // Window title
        char title[128];
        std::snprintf(title, sizeof(title), "MiauCore  |  %s  |  WoW 4.3.4 (15595)", applicationName);
        SetConsoleTitleA(title);

        // Modern monospace font (Cascadia Mono if present, Consolas otherwise)
        CONSOLE_FONT_INFOEX font = {};
        font.cbSize = sizeof(font);
        if (GetCurrentConsoleFontEx(out, FALSE, &font))
        {
            font.dwFontSize.X = 0;
            font.dwFontSize.Y = 17;
            font.FontFamily = FF_DONTCARE;
            font.FontWeight = FW_NORMAL;
            wcscpy_s(font.FaceName, L"Cascadia Mono");
            if (!SetCurrentConsoleFontEx(out, FALSE, &font))
            {
                wcscpy_s(font.FaceName, L"Consolas");
                SetCurrentConsoleFontEx(out, FALSE, &font);
            }
        }
        return true;
#else
        (void)applicationName;
        return isatty(fileno(stdout)) != 0;
#endif
    }
}

void Trinity::Banner::Show(char const* applicationName, void(*log)(char const* text), void(*logExtraInfo)())
{
    if (PrepareConsole(applicationName))
    {
        // Colored banner straight to the console; the log files get the plain version line only
        std::printf("\n");
        for (size_t i = 0; i < sizeof(BannerLines) / sizeof(BannerLines[0]); ++i)
            std::printf("\x1b[1;38;5;%dm%s\x1b[0m\n", GradientColors[i], BannerLines[i]);
        std::printf("\x1b[38;5;245m   ---------------------------------------------------------\x1b[0m\n");
        std::printf("\x1b[1;38;5;117m   %s\x1b[0m \x1b[38;5;250m|\x1b[0m \x1b[1;38;5;213mWoW Cataclysm 4.3.4\x1b[0m \x1b[38;5;250m| <Ctrl-C> para detener\x1b[0m\n\n", applicationName);
        std::fflush(stdout);
        log(Trinity::StringFormat("%s (%s)", GitRevision::GetFullVersion(), applicationName).c_str());
    }
    else
    {
        log(Trinity::StringFormat("%s (%s)", GitRevision::GetFullVersion(), applicationName).c_str());
        log("<Ctrl-C> to stop.\n");
        for (char const* line : BannerLines)
            log(line);
        log("");
    }

    if (logExtraInfo)
        logExtraInfo();
}
