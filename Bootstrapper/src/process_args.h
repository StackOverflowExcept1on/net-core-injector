#pragma once

// =============================================================================
//  Process arguments - retrieve command-line args from within a shared library.
// =============================================================================

#include <vector>
#include <string>

std::vector<std::string> get_process_args();
bool has_process_arg(const char* arg);
