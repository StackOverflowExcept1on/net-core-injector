// =============================================================================
//  Logging - file + console + OutputDebugString
// =============================================================================

#ifdef _WIN32
#include <windows.h>
#endif

#include <cstdio>
#include <cstdarg>

#include "logging.h"
#include "module_dir.h"

#ifndef BOOTSTRAPPER_LOG_NAME
#define BOOTSTRAPPER_LOG_NAME "bootstrapper.log" // Provided by CMake via target_compile_definitions; this is the IntelliSense fallback
#endif

// =============================================================================
//  Internal state
// =============================================================================

static bool g_loggingEnabled = false;
static FILE* g_logFile = nullptr;

void log_set_enabled(bool enabled) { g_loggingEnabled = enabled; }
bool log_is_enabled() { return g_loggingEnabled; }

// =============================================================================
//  Logging functions
// =============================================================================

void log_init() {
    if (!g_loggingEnabled) return;
    if (!g_logFile) {
        auto dir = get_module_directory();
#ifdef _WIN32
        auto logPath = dir + L"\\" + L"" BOOTSTRAPPER_LOG_NAME;
        g_logFile = _wfopen(logPath.c_str(), L"w");
#else
        auto logPath = dir + "/" + BOOTSTRAPPER_LOG_NAME;
        g_logFile = fopen(logPath.c_str(), "w");
#endif
    }
}

void log_printf(const char* fmt, ...) {
    if (!g_loggingEnabled) return;
    va_list args;

    if (g_logFile) {
        va_start(args, fmt);
        vfprintf(g_logFile, fmt, args);
        fflush(g_logFile);
        va_end(args);
    }

    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

#ifdef _WIN32
    char buf[512];
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    OutputDebugStringA(buf);
#endif
}
