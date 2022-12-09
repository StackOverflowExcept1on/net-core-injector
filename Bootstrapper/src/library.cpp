#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#include <windows.h>
#else
#define EXPORT __attribute__((visibility("default")))
#include <dlfcn.h>
#include <cstdio>
#endif

/// This class helps to manage shared libraries
class Module {
public:
    static void *getBaseAddress(const char *library) {
#ifdef _WIN32
        auto base = GetModuleHandleA(library);
#else
        auto base = dlopen(library, RTLD_LAZY);
#endif
        return reinterpret_cast<void *>(base);
    }

    static void *getExportByName(void *module, const char *name) {
#ifdef _WIN32
        auto address = GetProcAddress((HMODULE) module, name);
#else
        auto address = dlsym(module, name);
#endif
        return reinterpret_cast<void *>(address);
    }

    template<typename T>
    static T getFunctionByName(void *module, const char *name) {
        return reinterpret_cast<T>(getExportByName(module, name));
    }
};

#include <hostfxr.h>
#include <coreclr_delegates.h>

/// This enums represents possible errors to hide it from others
/// useful for debugging
enum class InitializeResult : uint32_t {
    Success,
    HostFxrLoadError,
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
    /// Get module base address
#ifdef _WIN32
    auto libraryName = "hostfxr.dll";
#else
    auto libraryName = "libhostfxr.so";
#endif
    void *module = Module::getBaseAddress(libraryName);
    if (!module) {
        return InitializeResult::HostFxrLoadError;
    }

    /// Obtaining useful exports
    auto hostfxr_initialize_for_runtime_config_fptr =
        Module::getFunctionByName<hostfxr_initialize_for_runtime_config_fn>(module, "hostfxr_initialize_for_runtime_config");

    auto hostfxr_get_runtime_delegate_fptr =
        Module::getFunctionByName<hostfxr_get_runtime_delegate_fn>(module, "hostfxr_get_runtime_delegate");

    auto hostfxr_close_fptr =
        Module::getFunctionByName<hostfxr_close_fn>(module, "hostfxr_close");

    /// Load runtime config
    hostfxr_handle ctx = nullptr;
    int rc = hostfxr_initialize_for_runtime_config_fptr(runtime_config_path, nullptr, &ctx);

    /// Success_HostAlreadyInitialized = 0x00000001
    /// @see https://github.com/dotnet/runtime/blob/main/docs/design/features/host-error-codes.md
    if (rc != 1 || ctx == nullptr) {
        hostfxr_close_fptr(ctx);
        return InitializeResult::InitializeRuntimeConfigError;
    }

    /// From docs: native function pointer to the requested runtime functionality
    void *delegate = nullptr;
    int ret = hostfxr_get_runtime_delegate_fptr(ctx, hostfxr_delegate_type::hdt_load_assembly_and_get_function_pointer,
                                                &delegate);

    if (ret != 0 || delegate == nullptr) {
        return InitializeResult::GetRuntimeDelegateError;
    }

    /// `void *` -> `load_assembly_and_get_function_pointer_fn`, undocumented???
    auto load_assembly_fptr = reinterpret_cast<load_assembly_and_get_function_pointer_fn>(delegate);

    typedef void (CORECLR_DELEGATE_CALLTYPE *custom_entry_point_fn)();
    custom_entry_point_fn custom = nullptr;

    ret = load_assembly_fptr(assembly_path, type_name, method_name, UNMANAGEDCALLERSONLY_METHOD, nullptr,
                             (void **) &custom);

    if (ret != 0 || custom == nullptr) {
        return InitializeResult::EntryPointError;
    }

    custom();

    hostfxr_close_fptr(ctx);

    return InitializeResult::Success;
}
