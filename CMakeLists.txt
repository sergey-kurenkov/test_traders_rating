cmake_minimum_required (VERSION 3.0)
project (traders_rating)

add_subdirectory(gtest)
set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "Enable testing of the benchmark library.")
add_subdirectory(benchmark)
find_package (Threads)
enable_testing()

set(CMAKE_CXX_FLAGS "-std=c++11 -pthread ${CMAKE_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
set(CMAKE_CXX_FLAGS_COVERAGE "-O0 -g --coverage")
set(CMAKE_C_FLAGS_COVERAGE "-O0 -g --coverage")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_EXE_LINKER_FLAGS_COVERAGE "${CMAKE_EXE_LINKER_FLAGS_COVERAGE} --coverage")

if(CMAKE_BUILD_TYPE STREQUAL "Coverage")
	if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
		file(COPY "${CMAKE_SOURCE_DIR}/scripts/llvm-gcov.sh" 
			 DESTINATION "${CMAKE_BINARY_DIR}/" 
			 FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
  			 	GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
		file(COPY "${CMAKE_SOURCE_DIR}/scripts/clang_coverage.sh" 
			 DESTINATION "${CMAKE_BINARY_DIR}/" 
			 FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
  			 	GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	else()
		file(COPY "${CMAKE_SOURCE_DIR}/scripts/gcc_coverage.sh" 
			 DESTINATION "${CMAKE_BINARY_DIR}/" 
			 FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ
  			 	GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
	endif()
endif()

include_directories("include")
include_directories("benchmark/include")
file(GLOB_RECURSE SOURCES "src/*.cpp" "include/*.h")
add_executable(traders_rating  ${SOURCES})
target_link_libraries(traders_rating ${CMAKE_THREAD_LIBS_INIT})

file(GLOB_RECURSE UNITTESTS_SOURCES "unittests/*.cpp" "src/traders_rating/*.cpp" "include/*.h")
add_executable(traders_rating_unit_tests  ${UNITTESTS_SOURCES})
target_link_libraries(traders_rating_unit_tests gtest ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME traders_rating_unit_tests COMMAND traders_rating_unit_tests)

file(GLOB_RECURSE PERFTESTS_SOURCES "perftests/*.cpp" "src/traders_rating/*.cpp" "include/*.h")
add_executable(traders_rating_perf_tests  ${PERFTESTS_SOURCES})
target_link_libraries(traders_rating_perf_tests benchmark ${CMAKE_THREAD_LIBS_INIT})
add_test(NAME traders_rating_perf_tests COMMAND traders_rating_perf_tests)

