cmake_minimum_required(VERSION 3.20)
project(Bootstrapper)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(${PROJECT_NAME} SHARED src/library.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include)

if (NOT WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE dl)
endif ()
