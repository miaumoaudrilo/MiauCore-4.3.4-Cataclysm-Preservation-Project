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

#include "AppenderConsole.h"
#include "LogMessage.h"
#include "StringFormat.h"
#include "StringConvert.h"
#include "Util.h"
#include <cstring>
#include <ctime>
#include <sstream>

#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
  #include <Windows.h>
#endif

AppenderConsole::AppenderConsole(uint8 id, std::string const& name, LogLevel level, AppenderFlags flags, std::vector<std::string_view> const& args)
    : Appender(id, name, level, flags), _colored(false)
{
    for (uint8 i = 0; i < NUM_ENABLED_LOG_LEVELS; ++i)
        _colors[i] = ColorTypes(NUM_COLOR_TYPES);

    if (3 < args.size())
        InitColors(name, args[3]);
}

void AppenderConsole::InitColors(std::string const& name, std::string_view str)
{
    if (str.empty())
    {
        _colored = false;
        return;
    }

    std::vector<std::string_view> colorStrs = Trinity::Tokenize(str, ' ', false);
    if (colorStrs.size() != NUM_ENABLED_LOG_LEVELS)
    {
        throw InvalidAppenderArgsException(Trinity::StringFormat("Log::CreateAppenderFromConfig: Invalid color data '%s' for console appender %s (expected %u entries, got %zu)",
            std::string(str).c_str(), name.c_str(), NUM_ENABLED_LOG_LEVELS, colorStrs.size()));
    }

    for (uint8 i = 0; i < NUM_ENABLED_LOG_LEVELS; ++i)
    {
        if (Optional<uint8> color = Trinity::StringTo<uint8>(colorStrs[i]); color && *color < NUM_COLOR_TYPES)
            _colors[i] = static_cast<ColorTypes>(*color);
        else
        {
            throw InvalidAppenderArgsException(Trinity::StringFormat("Log::CreateAppenderFromConfig: Invalid color '%u' for log level %s on console appender %s",
                std::string(colorStrs[i]).c_str(), static_cast<uint32>(i), name.c_str()));
        }
    }

    _colored = true;
}

void AppenderConsole::SetColor(bool stdout_stream, ColorTypes color)
{
#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
    static WORD WinColorFG[NUM_COLOR_TYPES] =
    {
        0,                                                  // BLACK
        FOREGROUND_RED,                                     // RED
        FOREGROUND_GREEN,                                   // GREEN
        FOREGROUND_RED | FOREGROUND_GREEN,                  // BROWN
        FOREGROUND_BLUE,                                    // BLUE
        FOREGROUND_RED |                    FOREGROUND_BLUE, // MAGENTA
        FOREGROUND_GREEN | FOREGROUND_BLUE,                 // CYAN
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, // WHITE
                                                            // YELLOW
        FOREGROUND_RED | FOREGROUND_GREEN |                   FOREGROUND_INTENSITY,
                                                            // RED_BOLD
        FOREGROUND_RED |                                      FOREGROUND_INTENSITY,
                                                            // GREEN_BOLD
        FOREGROUND_GREEN |                   FOREGROUND_INTENSITY,
        FOREGROUND_BLUE | FOREGROUND_INTENSITY,             // BLUE_BOLD
                                                            // MAGENTA_BOLD
        FOREGROUND_RED |                    FOREGROUND_BLUE | FOREGROUND_INTENSITY,
                                                            // CYAN_BOLD
        FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY,
                                                            // WHITE_BOLD
        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
    };

    HANDLE hConsole = GetStdHandle(stdout_stream ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);
    SetConsoleTextAttribute(hConsole, WinColorFG[color]);
#else
    enum ANSITextAttr
    {
        TA_NORMAL                                = 0,
        TA_BOLD                                  = 1,
        TA_BLINK                                 = 5,
        TA_REVERSE                               = 7
    };

    enum ANSIFgTextAttr
    {
        FG_BLACK                                 = 30,
        FG_RED,
        FG_GREEN,
        FG_BROWN,
        FG_BLUE,
        FG_MAGENTA,
        FG_CYAN,
        FG_WHITE,
        FG_YELLOW
    };

    enum ANSIBgTextAttr
    {
        BG_BLACK                                 = 40,
        BG_RED,
        BG_GREEN,
        BG_BROWN,
        BG_BLUE,
        BG_MAGENTA,
        BG_CYAN,
        BG_WHITE
    };

    static uint8 UnixColorFG[NUM_COLOR_TYPES] =
    {
        FG_BLACK,                                          // BLACK
        FG_RED,                                            // RED
        FG_GREEN,                                          // GREEN
        FG_BROWN,                                          // BROWN
        FG_BLUE,                                           // BLUE
        FG_MAGENTA,                                        // MAGENTA
        FG_CYAN,                                           // CYAN
        FG_WHITE,                                          // WHITE
        FG_YELLOW,                                         // YELLOW
        FG_RED,                                            // LRED
        FG_GREEN,                                          // LGREEN
        FG_BLUE,                                           // LBLUE
        FG_MAGENTA,                                        // LMAGENTA
        FG_CYAN,                                           // LCYAN
        FG_WHITE                                           // LWHITE
    };

    fprintf((stdout_stream? stdout : stderr), "\x1b[%d%sm", UnixColorFG[color], (color >= YELLOW && color < NUM_COLOR_TYPES ? ";1" : ""));
    #endif
}

void AppenderConsole::ResetColor(bool stdout_stream)
{
    #if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
    HANDLE hConsole = GetStdHandle(stdout_stream ? STD_OUTPUT_HANDLE : STD_ERROR_HANDLE);
    SetConsoleTextAttribute(hConsole, FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED);
    #else
    fprintf((stdout_stream ? stdout : stderr), "\x1b[0m");
    #endif
}

// MiauCore: consola moderna con secuencias VT (color de 24 bits). Formato:
//   12:34:56  INFO  | server.loading      | mensaje
// Hora en violeta, etiqueta de nivel con fondo de color, origen con color por categoría y texto según la gravedad.
namespace
{
    struct LevelStyle
    {
        char const* label;
        char const* badge;  // fondo + texto de la etiqueta
        char const* text;   // color del mensaje
    };

    LevelStyle const& StyleFor(LogLevel level)
    {
        static LevelStyle const styles[] =
        {
            { "TRACE", "\x1b[48;2;68;71;120m\x1b[38;2;210;215;255m", "\x1b[38;2;140;160;230m" },
            { "DEBUG", "\x1b[48;2;30;80;140m\x1b[38;2;200;235;255m", "\x1b[38;2;139;233;253m" },
            { "INFO ", "\x1b[48;2;18;87;68m\x1b[38;2;178;245;214m",  "\x1b[38;2;230;237;243m" },
            { "WARN ", "\x1b[48;2;120;84;0m\x1b[38;2;255;236;179m",  "\x1b[38;2;255;214;102m" },
            { "ERROR", "\x1b[48;2;140;26;34m\x1b[38;2;255;220;220m", "\x1b[38;2;255;123;114m" },
            { "FATAL", "\x1b[48;2;110;24;120m\x1b[38;2;255;220;255m", "\x1b[1;38;2;255;123;230m" },
        };
        switch (level)
        {
            case LOG_LEVEL_TRACE: return styles[0];
            case LOG_LEVEL_DEBUG: return styles[1];
            case LOG_LEVEL_INFO:  return styles[2];
            case LOG_LEVEL_WARN:  return styles[3];
            case LOG_LEVEL_ERROR: return styles[4];
            default:              return styles[5];
        }
    }

    // Color del origen según su categoría (primer tramo del nombre del logger)
    char const* OriginColor(std::string const& type)
    {
        struct Entry { char const* prefix; char const* color; };
        static Entry const palette[] =
        {
            { "server",    "\x1b[38;2;102;217;255m" }, // azul cielo
            { "sql",       "\x1b[38;2;255;170;80m"  }, // naranja
            { "scripts",   "\x1b[38;2;120;230;140m" }, // verde
            { "network",   "\x1b[38;2;255;121;198m" }, // rosa
            { "entities",  "\x1b[38;2;255;221;87m"  }, // amarillo
            { "spells",    "\x1b[38;2;189;147;249m" }, // morado
            { "maps",      "\x1b[38;2;80;250;200m"  }, // turquesa
            { "misc",      "\x1b[38;2;241;250;140m" }, // lima
            { "commands",  "\x1b[38;2;255;140;140m" }, // coral
            { "rbac",      "\x1b[38;2;255;140;140m" },
            { "chat",      "\x1b[38;2;140;200;255m" },
            { "loot",      "\x1b[38;2;255;200;120m" },
            { "achievement","\x1b[38;2;255;215;0m"  },
        };
        for (Entry const& e : palette)
            if (type.compare(0, std::strlen(e.prefix), e.prefix) == 0)
                return e.color;
        return "\x1b[38;2;139;233;253m";
    }

    bool EnableVirtualTerminal()
    {
#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
        bool ok = true;
        for (DWORD handleId : { STD_OUTPUT_HANDLE, STD_ERROR_HANDLE })
        {
            HANDLE h = GetStdHandle(handleId);
            DWORD mode = 0;
            if (h == INVALID_HANDLE_VALUE || h == nullptr || !GetConsoleMode(h, &mode)
                || !SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING | ENABLE_PROCESSED_OUTPUT))
                ok = false;
        }
        return ok;
#else
        return true;
#endif
    }
}

void AppenderConsole::_write(LogMessage const* message)
{
    bool stdout_stream = !(message->level == LOG_LEVEL_ERROR || message->level == LOG_LEVEL_FATAL);
    FILE* stream = stdout_stream ? stdout : stderr;

    static bool const vt = EnableVirtualTerminal();
    if (_colored && vt)
    {
        LevelStyle const& style = StyleFor(message->level);

        char timeBuf[16] = {};
        tm localTm;
#if TRINITY_PLATFORM == TRINITY_PLATFORM_WINDOWS
        localtime_s(&localTm, &message->mtime);
#else
        localtime_r(&message->mtime, &localTm);
#endif
        std::strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &localTm);

        // origen del log recortado/alineado a 18 columnas
        std::string origin = message->type;
        if (origin.size() > 18)
            origin = origin.substr(origin.size() - 18);
        origin.resize(18, ' ');

        utf8printf(stream, "\x1b[38;2;179;146;240m%s\x1b[0m %s %s \x1b[0m \x1b[38;2;255;121;198m|\x1b[0m %s%s\x1b[0m \x1b[38;2;255;121;198m|\x1b[0m %s%s\x1b[0m\n",
            timeBuf, style.badge, style.label, OriginColor(message->type), origin.c_str(), style.text, message->text.c_str());
        return;
    }

    if (_colored)
    {
        uint8 index;
        switch (message->level)
        {
            case LOG_LEVEL_TRACE: index = 5; break;
            case LOG_LEVEL_DEBUG: index = 4; break;
            case LOG_LEVEL_INFO:  index = 3; break;
            case LOG_LEVEL_WARN:  index = 2; break;
            case LOG_LEVEL_FATAL: index = 0; break;
            case LOG_LEVEL_ERROR: [[fallthrough]];
            default:              index = 1; break;
        }

        SetColor(stdout_stream, _colors[index]);
        utf8printf(stream, "%s%s\n", message->prefix.c_str(), message->text.c_str());
        ResetColor(stdout_stream);
    }
    else
        utf8printf(stream, "%s%s\n", message->prefix.c_str(), message->text.c_str());
}
