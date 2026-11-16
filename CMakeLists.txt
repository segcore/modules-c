# Who knows the maximum. But its all simple stuff, so hopefull it works for a long time.
cmake_minimum_required(VERSION 3.0...5.99)

project(modules_c)

add_library(network STATIC network.c)

option(BUILD_EXAMPLES "Build example programs?" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
