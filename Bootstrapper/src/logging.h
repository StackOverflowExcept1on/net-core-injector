#pragma once

// =============================================================================
//  Logging - file + console + OutputDebugString
// =============================================================================

void log_set_enabled(bool enabled);
bool log_is_enabled();
void log_init();
void log_printf(const char* fmt, ...);
