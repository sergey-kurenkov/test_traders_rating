cmake_minimum_required (VERSION 3.0)
project (traders_rating)
include(gtest.cmake)
find_package (Threads)
enable_testing()

set (CMAKE_CXX_FLAGS "-std=c++11 ${CMAKE_CXX_FLAGS}")
include_directories("src")
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")
add_executable(traders_rating  ${SOURCES})

add_executable(traders_rating_unit_tests  ${SOURCES})
target_link_libraries(traders_rating_unit_tests gtest ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME traders_rating_unit_tests COMMAND traders_rating_unit_tests)
