# Who knows the maximum. But its all simple stuff, so hopefully it works for a long time.
cmake_minimum_required(VERSION 3.0...5.99)

project(modules_c LANGUAGES C)


if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    option(MODULES_C_COVERAGE "Enable code coverage" On)
    if (MODULES_C_COVERAGE)
        add_compile_options(--coverage)
        add_link_options(--coverage)
    endif()
endif()

add_library(network STATIC network.c)

option(BUILD_EXAMPLES "Build example programs?" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
