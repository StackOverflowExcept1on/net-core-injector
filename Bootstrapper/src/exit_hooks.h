#pragma once

// =============================================================================
//  Exit hooks - prevent premature process termination (x86_64)
//  Provides hook_exit_process() / unhook_exit_process() on all platforms.
// =============================================================================

void exit_hooks_set_enabled(bool enabled);
bool exit_hooks_is_enabled();
void hook_exit_process();
void unhook_exit_process();
