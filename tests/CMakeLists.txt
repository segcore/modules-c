# Who knows the maximum. But its all simple stuff, so hopefull it works for a long time.
cmake_minimum_required(VERSION 3.0...5.99)

# So this directory can be used directly by `cmake`
project(modules_c_tests)
include(CTest)

# Function to create tests
function(test target_name)
    add_executable("${target_name}" "${ARGN}")
    target_include_directories("${target_name}" PRIVATE ..)
    add_test(NAME "${target_name}" COMMAND "${target_name}")
endfunction()

# testing.h tests
test(testing_h_positive ./testing_h_positive.c)
test(testing_h_negative ./testing_h_negative.c)
test(testing_h_exit ./testing_h_exit.c)
set_property(TEST testing_h_exit PROPERTY WILL_FAIL true)

test(test_network          ./test_network.c)
test(test_network_shutdown ./test_network_shutdown.c)
target_link_libraries(test_network          PRIVATE network)
target_link_libraries(test_network_shutdown PRIVATE network)
