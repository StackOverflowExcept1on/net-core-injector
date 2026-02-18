#pragma once

// =============================================================================
//  Module directory resolution - returns the directory containing this DLL/SO.
// =============================================================================

#ifdef _WIN32
#include <windows.h>
#include <string>
void set_module_handle(HINSTANCE hInstance);
std::wstring get_module_directory();
#else
#include <string>
std::string get_module_directory();
#endif
