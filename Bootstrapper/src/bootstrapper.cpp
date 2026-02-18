// =============================================================================
//  Platform includes & macros
// =============================================================================

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#include <windows.h>
#else
#define EXPORT [[gnu::visibility("default")]]
#include <dlfcn.h>
#endif

#include <string>
#include <chrono>
#include <thread>
#include <cstdlib>

#include <hostfxr.h>
#include <coreclr_delegates.h>

#include "logging.h"
#include "module_dir.h"
#include "exit_hooks.h"
#include "process_args.h"

// =============================================================================
//  Configuration - predefined paths for automatic initialization
// =============================================================================

#ifndef RUNTIME_CONFIG_PATH
#define RUNTIME_CONFIG_PATH "RuntimePatcher.runtimeconfig.json"
#endif

#ifndef ASSEMBLY_PATH
#define ASSEMBLY_PATH "RuntimePatcher.dll"
#endif

#ifndef TYPE_NAME
#define TYPE_NAME "RuntimePatcher.Main, RuntimePatcher"
#endif

#ifndef METHOD_NAME
#define METHOD_NAME "InitializePatchesUnmanaged"
#endif

#ifndef HOSTFXR_TIMEOUT_MS
#define HOSTFXR_TIMEOUT_MS 30000
#endif

#ifndef HOSTFXR_POLL_INTERVAL_MS
#define HOSTFXR_POLL_INTERVAL_MS 100
#endif

#ifdef _WIN32
#define HOSTFXR_LIBRARY_NAME "hostfxr.dll"
#else
#define HOSTFXR_LIBRARY_NAME "libhostfxr.so"
#endif

// =============================================================================
//  Module - cross-platform shared library helpers
// =============================================================================

class Module {
public:
    static void *get_base_address(const char *library) {
#ifdef _WIN32
        auto base = GetModuleHandleA(library);
#else
        auto base = dlopen(library, RTLD_LAZY);
#endif
        return reinterpret_cast<void *>(base);
    }

    static void *get_export_by_name(void *module, const char *name) {
#ifdef _WIN32
        auto address = GetProcAddress((HMODULE) module, name);
#else
        auto address = dlsym(module, name);
#endif
        return reinterpret_cast<void *>(address);
    }

    template<typename T>
    static T get_function_by_name(void *module, const char *name) {
        return reinterpret_cast<T>(get_export_by_name(module, name));
    }
};

// =============================================================================
//  Core - .NET assembly loading via hostfxr
// =============================================================================

enum class InitializeResult : uint32_t {
    Success,
    HostFxrLoadError,
    HostFxrFptrLoadError,
    InitializeRuntimeConfigError,
    GetRuntimeDelegateError,
    EntryPointError,
};

extern "C" EXPORT InitializeResult bootstrapper_load_assembly(
    const char_t *runtime_config_path,
    const char_t *assembly_path,
    const char_t *type_name,
    const char_t *method_name
) {
    void *module = Module::get_base_address(HOSTFXR_LIBRARY_NAME);
    if (!module) {
        log_printf("[-] Failed to load %s\n", HOSTFXR_LIBRARY_NAME);
        return InitializeResult::HostFxrLoadError;
    }

#ifdef _WIN32
    log_printf("[*] runtime_config_path: %ls\n", runtime_config_path);
    log_printf("[*] assembly_path:       %ls\n", assembly_path);
    log_printf("[*] type_name:           %ls\n", type_name);
    log_printf("[*] method_name:         %ls\n", method_name);
#else
    log_printf("[*] runtime_config_path: %s\n", runtime_config_path);
    log_printf("[*] assembly_path:       %s\n", assembly_path);
    log_printf("[*] type_name:           %s\n", type_name);
    log_printf("[*] method_name:         %s\n", method_name);
#endif

    auto hostfxr_initialize_for_runtime_config_fptr =
        Module::get_function_by_name<hostfxr_initialize_for_runtime_config_fn>(module, "hostfxr_initialize_for_runtime_config");

    auto hostfxr_get_runtime_delegate_fptr =
        Module::get_function_by_name<hostfxr_get_runtime_delegate_fn>(module, "hostfxr_get_runtime_delegate");

    auto hostfxr_close_fptr =
        Module::get_function_by_name<hostfxr_close_fn>(module, "hostfxr_close");

    if (!hostfxr_initialize_for_runtime_config_fptr || !hostfxr_get_runtime_delegate_fptr || !hostfxr_close_fptr) {
        log_printf("[-] Failed to resolve hostfxr exports (init=%p, delegate=%p, close=%p)\n",
                   (void*)hostfxr_initialize_for_runtime_config_fptr,
                   (void*)hostfxr_get_runtime_delegate_fptr,
                   (void*)hostfxr_close_fptr);
        return InitializeResult::HostFxrFptrLoadError;
    }

    hostfxr_handle ctx = nullptr;
    int rc = hostfxr_initialize_for_runtime_config_fptr(runtime_config_path, nullptr, &ctx);
    log_printf("[*] hostfxr_initialize_for_runtime_config => 0x%08X\n", (unsigned int)rc);

    /// Success = 0x00000000
    /// Success_HostAlreadyInitialized = 0x00000001
    /// @see https://github.com/dotnet/runtime/blob/main/docs/design/features/host-error-codes.md
    if (rc != 1 || ctx == nullptr) {
        hostfxr_close_fptr(ctx);
        return InitializeResult::InitializeRuntimeConfigError;
    }

    void *delegate = nullptr;
    int ret = hostfxr_get_runtime_delegate_fptr(ctx, hostfxr_delegate_type::hdt_load_assembly_and_get_function_pointer,
                                                &delegate);

    if (ret != 0 || delegate == nullptr) {
        log_printf("[-] hostfxr_get_runtime_delegate failed => 0x%08X\n", (unsigned int)ret);
        return InitializeResult::GetRuntimeDelegateError;
    }

    auto load_assembly_fptr = reinterpret_cast<load_assembly_and_get_function_pointer_fn>(delegate);

    typedef void (CORECLR_DELEGATE_CALLTYPE *custom_entry_point_fn)();
    custom_entry_point_fn custom = nullptr;

    ret = load_assembly_fptr(assembly_path, type_name, method_name, UNMANAGEDCALLERSONLY_METHOD, nullptr,
                             (void **) &custom);

    if (ret != 0 || custom == nullptr) {
        log_printf("[-] load_assembly_and_get_function_pointer failed => 0x%08X\n", (unsigned int)ret);
        return InitializeResult::EntryPointError;
    }

    log_printf("[+] Invoking managed entry point\n");
    custom();

    hostfxr_close_fptr(ctx);

    return InitializeResult::Success;
}

// =============================================================================
//  Initialization - hostfxr polling & assembly loading orchestration
// =============================================================================

static bool wait_for_hostfxr(int timeout_ms) {
    int elapsed = 0;
    while (!Module::get_base_address(HOSTFXR_LIBRARY_NAME) && elapsed < timeout_ms) {
        std::this_thread::sleep_for(std::chrono::milliseconds(HOSTFXR_POLL_INTERVAL_MS));
        elapsed += HOSTFXR_POLL_INTERVAL_MS;
    }

    if (!Module::get_base_address(HOSTFXR_LIBRARY_NAME)) {
        log_printf("[-] hostfxr not loaded after %dms, aborting\n", timeout_ms);
        return false;
    }

    log_printf("[+] hostfxr found after ~%dms\n", elapsed);
    return true;
}

static std::string get_env_var(const char *name) {
    auto val = std::getenv(name);
    return val == nullptr ? std::string() : std::string(val);
}

static void initialize_from_env(int timeout_ms) {
    auto runtime_config_path = get_env_var("RUNTIME_CONFIG_PATH");
    auto assembly_path = get_env_var("ASSEMBLY_PATH");
    auto type_name = get_env_var("TYPE_NAME");
    auto method_name = get_env_var("METHOD_NAME");

    if (runtime_config_path.empty() || assembly_path.empty() || type_name.empty() || method_name.empty()) {
        return;
    }

    if (!wait_for_hostfxr(timeout_ms)) return;

    auto ret = bootstrapper_load_assembly(
#ifdef _WIN32
        std::wstring(runtime_config_path.begin(), runtime_config_path.end()).c_str(),
        std::wstring(assembly_path.begin(), assembly_path.end()).c_str(),
        std::wstring(type_name.begin(), type_name.end()).c_str(),
        std::wstring(method_name.begin(), method_name.end()).c_str()
#else
        runtime_config_path.c_str(),
        assembly_path.c_str(),
        type_name.c_str(),
        method_name.c_str()
#endif
    );
    log_printf("[+] bootstrapper_load_assembly() => %d\n", (uint32_t) ret);
}

static void initialize_from_constants(int timeout_ms) {
    if (!wait_for_hostfxr(timeout_ms)) return;

    auto dir = get_module_directory();

#ifdef _WIN32
    auto runtime_config_path = dir + L"\\" + L"" RUNTIME_CONFIG_PATH;
    auto assembly_path       = dir + L"\\" + L"" ASSEMBLY_PATH;
#else
    auto runtime_config_path = dir + "/" + RUNTIME_CONFIG_PATH;
    auto assembly_path       = dir + "/" + ASSEMBLY_PATH;
#endif

    auto ret = bootstrapper_load_assembly(
#ifdef _WIN32
        runtime_config_path.c_str(),
        assembly_path.c_str(),
        L"" TYPE_NAME,
        L"" METHOD_NAME
#else
        runtime_config_path.c_str(),
        assembly_path.c_str(),
        TYPE_NAME,
        METHOD_NAME
#endif
    );
    log_printf("[+] bootstrapper_load_assembly() => %d\n", (uint32_t) ret);
}

// =============================================================================
//  Entry point - DllMain / constructor
// =============================================================================

#ifndef DEFAULT_LOGGING_ENABLED
#define DEFAULT_LOGGING_ENABLED 1 // Provided by CMake; this is the IntelliSense fallback
#endif

#ifndef DEFAULT_EXIT_HOOKS_ENABLED
#define DEFAULT_EXIT_HOOKS_ENABLED 1 // Provided by CMake; this is the IntelliSense fallback
#endif

/// Applies CMake defaults, then overrides with command-line args if present.
static void configure_features() {
    log_set_enabled(DEFAULT_LOGGING_ENABLED);
    exit_hooks_set_enabled(DEFAULT_EXIT_HOOKS_ENABLED);

    if (has_process_arg("-LogBootstrapper"))  log_set_enabled(true);
    if (has_process_arg("-HookExitProcess"))  exit_hooks_set_enabled(true);

    log_init();
    hook_exit_process();

    // Log the command line after logging is initialized
    auto args = get_process_args();
    log_printf("[*] Command line (%zu args):\n", args.size());
    for (size_t i = 0; i < args.size(); i++) {
        log_printf("[*]   [%zu] %s\n", i, args[i].c_str());
    }
}

#ifdef _WIN32
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    if (fdwReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hinstDLL);
        set_module_handle(hinstDLL);

        configure_features();

        CreateThread(nullptr, 0, [](LPVOID) -> DWORD {
            initialize_from_constants(HOSTFXR_TIMEOUT_MS);
            return 0;
        }, nullptr, 0, nullptr);
    }
    return TRUE;
}
#else
[[gnu::constructor]]
void initialize_library() {
    configure_features();

    std::thread thread([] {
        initialize_from_constants(HOSTFXR_TIMEOUT_MS);
    });
    thread.detach();
}
#endif
