// =============================================================================
//  Module directory resolution
// =============================================================================

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#endif

#include <string>

#include "module_dir.h"

// =============================================================================
//  Internal state
// =============================================================================

#ifdef _WIN32
static HINSTANCE g_hInstance = nullptr;

void set_module_handle(HINSTANCE hInstance) {
    g_hInstance = hInstance;
}
#endif

// =============================================================================
//  Directory resolution
// =============================================================================

#ifdef _WIN32
std::wstring get_module_directory() {
    wchar_t path[MAX_PATH];
    DWORD len = GetModuleFileNameW(g_hInstance, path, MAX_PATH);
    if (len == 0) return L".";
    std::wstring dir(path, len);
    auto pos = dir.find_last_of(L"\\//");
    return (pos != std::wstring::npos) ? dir.substr(0, pos) : L".";
}
#else
std::string get_module_directory() {
    Dl_info info;
    if (dladdr((void*)&get_module_directory, &info) && info.dli_fname) {
        std::string path(info.dli_fname);
        auto pos = path.find_last_of('/');
        return (pos != std::string::npos) ? path.substr(0, pos) : ".";
    }
    return ".";
}
#endif
