// =============================================================================
//  Exit hooks - prevent premature process termination (x86_64)
// =============================================================================

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#endif

#include <cstdint>
#include <cstring>

#include "exit_hooks.h"
#include "logging.h"

static bool g_exitHooksEnabled = false;

void exit_hooks_set_enabled(bool enabled) { g_exitHooksEnabled = enabled; }
bool exit_hooks_is_enabled() { return g_exitHooksEnabled; }

// =============================================================================
//  Inline hook primitives
// =============================================================================

#ifdef _WIN32
typedef BYTE HookByte;
#else
typedef unsigned char HookByte;
#endif

static constexpr int HOOK_SIZE = 14;

/// Writes a 14-byte inline hook (MOV RAX, imm64; JMP RAX) - x86_64 only
static void write_inline_hook(void* target, void* detour, HookByte* backup) {
#ifdef _WIN32
    DWORD oldProtect;
    VirtualProtect(target, HOOK_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect);
#else
    long pagesize = sysconf(_SC_PAGESIZE);
    uintptr_t pageStart = (uintptr_t)target & ~(pagesize - 1);
    size_t len = ((uintptr_t)target + HOOK_SIZE) - pageStart;
    mprotect((void*)pageStart, len, PROT_READ | PROT_WRITE | PROT_EXEC);
#endif

    if (backup) memcpy(backup, target, HOOK_SIZE);

    HookByte hook[HOOK_SIZE] = { 0 };
    hook[0] = 0x48; hook[1] = 0xB8; // MOV RAX, imm64
    *(uintptr_t*)(hook + 2) = (uintptr_t)detour;
    hook[10] = 0xFF; hook[11] = 0xE0; // JMP RAX
    memcpy(target, hook, HOOK_SIZE);

#ifdef _WIN32
    VirtualProtect(target, HOOK_SIZE, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target, HOOK_SIZE);
#else
    mprotect((void*)pageStart, len, PROT_READ | PROT_EXEC);
    __builtin___clear_cache((char*)target, (char*)target + HOOK_SIZE);
#endif
}

static void restore_inline_hook(void* target, const HookByte* backup) {
    if (!target || !backup) return;
#ifdef _WIN32
    DWORD oldProtect;
    VirtualProtect(target, HOOK_SIZE, PAGE_EXECUTE_READWRITE, &oldProtect);
    memcpy(target, backup, HOOK_SIZE);
    VirtualProtect(target, HOOK_SIZE, oldProtect, &oldProtect);
    FlushInstructionCache(GetCurrentProcess(), target, HOOK_SIZE);
#else
    long pagesize = sysconf(_SC_PAGESIZE);
    uintptr_t pageStart = (uintptr_t)target & ~(pagesize - 1);
    size_t len = ((uintptr_t)target + HOOK_SIZE) - pageStart;
    mprotect((void*)pageStart, len, PROT_READ | PROT_WRITE | PROT_EXEC);
    memcpy(target, backup, HOOK_SIZE);
    mprotect((void*)pageStart, len, PROT_READ | PROT_EXEC);
    __builtin___clear_cache((char*)target, (char*)target + HOOK_SIZE);
#endif
}

// =============================================================================
//  Hook state
// =============================================================================

static HookByte g_originalExitBytes[HOOK_SIZE] = {};
static HookByte g_originalTerminateBytes[HOOK_SIZE] = {};
static void* g_exitAddr = nullptr;
static void* g_terminateAddr = nullptr;

// =============================================================================
//  Platform-specific hooked functions & hook/unhook logic
// =============================================================================

#ifdef _WIN32

static void WINAPI hooked_exit_process(UINT uExitCode) {
    log_printf("[!] ExitProcess(%u) intercepted - suspending caller thread\n", uExitCode);
    SuspendThread(GetCurrentThread());
    Sleep(INFINITE);
}

static BOOL WINAPI hooked_terminate_process(HANDLE hProcess, UINT uExitCode) {
    if (hProcess == GetCurrentProcess() || hProcess == (HANDLE)-1) {
        log_printf("[!] TerminateProcess(%u) intercepted - suspending caller thread\n", uExitCode);
        SuspendThread(GetCurrentThread());
        Sleep(INFINITE);
        return TRUE;
    }
    restore_inline_hook(g_terminateAddr, g_originalTerminateBytes);
    BOOL result = TerminateProcess(hProcess, uExitCode);
    write_inline_hook(g_terminateAddr, (void*)&hooked_terminate_process, nullptr);
    return result;
}

void hook_exit_process() {
    if (!g_exitHooksEnabled) return;
    g_exitAddr = (void*)GetProcAddress(GetModuleHandleA("kernel32.dll"), "ExitProcess");
    if (g_exitAddr) {
        write_inline_hook(g_exitAddr, (void*)&hooked_exit_process, g_originalExitBytes);
        log_printf("[+] ExitProcess hooked\n");
    } else {
        log_printf("[-] Failed to find ExitProcess\n");
    }

    g_terminateAddr = (void*)GetProcAddress(GetModuleHandleA("kernel32.dll"), "TerminateProcess");
    if (g_terminateAddr) {
        write_inline_hook(g_terminateAddr, (void*)&hooked_terminate_process, g_originalTerminateBytes);
        log_printf("[+] TerminateProcess hooked\n");
    } else {
        log_printf("[-] Failed to find TerminateProcess\n");
    }
}

void unhook_exit_process() {
    restore_inline_hook(g_exitAddr, g_originalExitBytes);
    restore_inline_hook(g_terminateAddr, g_originalTerminateBytes);
    log_printf("[+] Exit hooks removed\n");
}

#else // Linux / macOS

static void hooked_exit(int status) {
    log_printf("[!] exit(%d) intercepted - blocking caller thread\n", status);
    while (true) pause();
}

static void hooked_underscore_exit(int status) {
    log_printf("[!] _exit(%d) intercepted - blocking caller thread\n", status);
    while (true) pause();
}

void hook_exit_process() {
    if (!g_exitHooksEnabled) return;
    g_exitAddr = dlsym(RTLD_DEFAULT, "exit");
    if (g_exitAddr) {
        write_inline_hook(g_exitAddr, (void*)&hooked_exit, g_originalExitBytes);
        log_printf("[+] exit() hooked\n");
    } else {
        log_printf("[-] Failed to find exit()\n");
    }

    g_terminateAddr = dlsym(RTLD_DEFAULT, "_exit");
    if (g_terminateAddr) {
        write_inline_hook(g_terminateAddr, (void*)&hooked_underscore_exit, g_originalTerminateBytes);
        log_printf("[+] _exit() hooked\n");
    } else {
        log_printf("[-] Failed to find _exit()\n");
    }
}

void unhook_exit_process() {
    restore_inline_hook(g_exitAddr, g_originalExitBytes);
    restore_inline_hook(g_terminateAddr, g_originalTerminateBytes);
    log_printf("[+] Exit hooks removed\n");
}

#endif
