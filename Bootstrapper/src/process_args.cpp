// =============================================================================
//  Process arguments - retrieve command-line args from within a shared library.
// =============================================================================

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <cstdio>
#include <cstring>
#endif

#include "process_args.h"

std::vector<std::string> get_process_args() {
    std::vector<std::string> args;

#ifdef _WIN32
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (argv) {
        for (int i = 0; i < argc; i++) {
            int size = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
            std::string s(size - 1, '\0');
            WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, s.data(), size, nullptr, nullptr);
            args.push_back(std::move(s));
        }
        LocalFree(argv);
    }
#else // Linux
    FILE* f = fopen("/proc/self/cmdline", "r");
    if (f) {
        char buf[4096];
        size_t len = fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        for (size_t i = 0; i < len;) {
            args.emplace_back(buf + i);
            i += args.back().size() + 1;
        }
    }
#endif

    return args;
}

bool has_process_arg(const char* arg) {
    for (const auto& a : get_process_args()) {
#ifdef _WIN32
        if (_stricmp(a.c_str(), arg) == 0) return true;
#else
        if (strcasecmp(a.c_str(), arg) == 0) return true;
#endif
    }
    return false;
}
