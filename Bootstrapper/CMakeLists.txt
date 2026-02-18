cmake_minimum_required(VERSION 3.20)
project(Bootstrapper)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(DEFAULT_LOGGING_ENABLED "Enable logging by default (overridden by -LogBootstrapper arg)" OFF)
option(DEFAULT_EXIT_HOOKS_ENABLED "Enable exit hooks by default (overridden by -HookExitProcess arg)" OFF)

add_library(${PROJECT_NAME} SHARED src/bootstrapper.cpp src/module_dir.cpp src/logging.cpp src/exit_hooks.cpp src/process_args.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include)
target_compile_definitions(${PROJECT_NAME} PRIVATE BOOTSTRAPPER_LOG_NAME="${PROJECT_NAME}.log")

if (DEFAULT_LOGGING_ENABLED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_LOGGING_ENABLED=1)
else ()
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_LOGGING_ENABLED=0)
endif ()

if (DEFAULT_EXIT_HOOKS_ENABLED)
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_EXIT_HOOKS_ENABLED=1)
else ()
    target_compile_definitions(${PROJECT_NAME} PRIVATE DEFAULT_EXIT_HOOKS_ENABLED=0)
endif ()

if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif ()

if (WIN32)
    add_library(VersionProxy SHARED src/version_proxy.cpp src/version_proxy.def)
    set_target_properties(VersionProxy PROPERTIES OUTPUT_NAME "VERSION")
    target_compile_definitions(VersionProxy PRIVATE BOOTSTRAPPER_DLL_NAME="$<TARGET_FILE_NAME:${PROJECT_NAME}>")
    install(TARGETS VersionProxy DESTINATION ${CMAKE_BINARY_DIR}/bin)
endif ()

install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_BINARY_DIR}/bin)
