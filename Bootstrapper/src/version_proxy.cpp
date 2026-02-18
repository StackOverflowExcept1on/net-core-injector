// version_proxy.cpp - compile as VERSION.dll
#define WIN32_LEAN_AND_MEAN
#define NOGDI
#include <windows.h>
#include <cstdio>

#ifndef BOOTSTRAPPER_DLL_NAME
#define BOOTSTRAPPER_DLL_NAME "Bootstrapper.dll" // Provided by CMake via target_compile_definitions; this is the IntelliSense fallback
#endif

static HMODULE hRealVersion = nullptr;

static void loadRealVersion() {
    if (hRealVersion) return;
    char systemDir[MAX_PATH];
    GetSystemDirectoryA(systemDir, MAX_PATH);
    strcat_s(systemDir, "\\version.dll");
    hRealVersion = LoadLibraryA(systemDir);
}

static FARPROC getReal(const char *name) {
    loadRealVersion();
    return GetProcAddress(hRealVersion, name);
}

/// Each proxy function uses __VA_ARGS__ passthrough via a void* cast
/// We declare them with unique names to avoid conflicts with winver.h

#define DEFINE_PROXY(name) \
    extern "C" __declspec(naked) void proxy_##name() { \
        __asm { jmp [pfn_##name] } \
    }

// MSVC x64 doesn't support __asm. Use a different approach for x64:

extern "C" {
    // Pointers to real functions, resolved at load time
    FARPROC pfn_GetFileVersionInfoA;
    FARPROC pfn_GetFileVersionInfoByHandle;
    FARPROC pfn_GetFileVersionInfoExA;
    FARPROC pfn_GetFileVersionInfoExW;
    FARPROC pfn_GetFileVersionInfoSizeA;
    FARPROC pfn_GetFileVersionInfoSizeExA;
    FARPROC pfn_GetFileVersionInfoSizeExW;
    FARPROC pfn_GetFileVersionInfoSizeW;
    FARPROC pfn_GetFileVersionInfoW;
    FARPROC pfn_VerFindFileA;
    FARPROC pfn_VerFindFileW;
    FARPROC pfn_VerInstallFileA;
    FARPROC pfn_VerInstallFileW;
    FARPROC pfn_VerLanguageNameA;
    FARPROC pfn_VerLanguageNameW;
    FARPROC pfn_VerQueryValueA;
    FARPROC pfn_VerQueryValueW;
}

static void resolveAll() {
    loadRealVersion();
    pfn_GetFileVersionInfoA        = getReal("GetFileVersionInfoA");
    pfn_GetFileVersionInfoByHandle = getReal("GetFileVersionInfoByHandle");
    pfn_GetFileVersionInfoExA      = getReal("GetFileVersionInfoExA");
    pfn_GetFileVersionInfoExW      = getReal("GetFileVersionInfoExW");
    pfn_GetFileVersionInfoSizeA    = getReal("GetFileVersionInfoSizeA");
    pfn_GetFileVersionInfoSizeExA  = getReal("GetFileVersionInfoSizeExA");
    pfn_GetFileVersionInfoSizeExW  = getReal("GetFileVersionInfoSizeExW");
    pfn_GetFileVersionInfoSizeW    = getReal("GetFileVersionInfoSizeW");
    pfn_GetFileVersionInfoW        = getReal("GetFileVersionInfoW");
    pfn_VerFindFileA               = getReal("VerFindFileA");
    pfn_VerFindFileW               = getReal("VerFindFileW");
    pfn_VerInstallFileA            = getReal("VerInstallFileA");
    pfn_VerInstallFileW            = getReal("VerInstallFileW");
    pfn_VerLanguageNameA           = getReal("VerLanguageNameA");
    pfn_VerLanguageNameW           = getReal("VerLanguageNameW");
    pfn_VerQueryValueA             = getReal("VerQueryValueA");
    pfn_VerQueryValueW             = getReal("VerQueryValueW");
}

/// Proxy functions - just call through the function pointer
/// Using void* variadic trick: on x64, args stay in registers, so a simple call-through works

#define PROXY(name) \
    extern "C" void* __cdecl proxy_##name() { \
        return ((void*(*)())pfn_##name)(); \
    }

PROXY(GetFileVersionInfoA)
PROXY(GetFileVersionInfoByHandle)
PROXY(GetFileVersionInfoExA)
PROXY(GetFileVersionInfoExW)
PROXY(GetFileVersionInfoSizeA)
PROXY(GetFileVersionInfoSizeExA)
PROXY(GetFileVersionInfoSizeExW)
PROXY(GetFileVersionInfoSizeW)
PROXY(GetFileVersionInfoW)
PROXY(VerFindFileA)
PROXY(VerFindFileW)
PROXY(VerInstallFileA)
PROXY(VerInstallFileW)
PROXY(VerLanguageNameA)
PROXY(VerLanguageNameW)
PROXY(VerQueryValueA)
PROXY(VerQueryValueW)

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        resolveAll();
        LoadLibraryA(BOOTSTRAPPER_DLL_NAME);
    }
    if (fdwReason == DLL_PROCESS_DETACH) {
        if (hRealVersion) FreeLibrary(hRealVersion);
    }
    return TRUE;
}
