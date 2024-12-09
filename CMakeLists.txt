cmake_minimum_required(VERSION 3.20)
project("3D_From_Scratch")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(program_executable_name ${CMAKE_PROJECT_NAME})
set(test_executable_name ${CMAKE_PROJECT_NAME}_TEST)
set(bench_executable_name ${CMAKE_PROJECT_NAME}_BENCH)

file(GLOB_RECURSE src_files CONFIGURE_DEPENDS src/*.cpp)
file(GLOB_RECURSE test_files CONFIGURE_DEPENDS tests/*.cpp src/*.cpp)
list(REMOVE_ITEM test_files ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)
file(GLOB_RECURSE bench_files CONFIGURE_DEPENDS benchmarks/*.cpp src/*.cpp)
list(REMOVE_ITEM bench_files ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
set(compile_options
  "$<${gcc_like_cxx}:-pedantic-errors;-Wall;-Weffc++;-Wextra;-Wsign-conversion;-Wconversion>"
  "$<${msvc_cxx}:/permissive-;/W4;/WX>"
)

find_package(benchmark REQUIRED)
find_package(glm REQUIRED)
find_package(SFML COMPONENTS graphics REQUIRED)

include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/f063cd25c90cbd4089a0ff96f5991df4f2721338.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

add_executable(${program_executable_name} ${src_files})
target_include_directories(${program_executable_name} PRIVATE include)
target_compile_options(${program_executable_name} PRIVATE ${compile_options})
target_link_libraries(
  ${program_executable_name}
  glm::glm
  sfml-graphics
)

add_executable(${test_executable_name} ${test_files})
target_include_directories(${test_executable_name} PRIVATE include)
target_compile_options(${test_executable_name} PRIVATE ${compile_options})
target_link_libraries(
  ${test_executable_name}
  GTest::gtest_main
  glm::glm
  sfml-graphics
)

add_executable(${bench_executable_name} ${bench_files})
target_include_directories(${bench_executable_name} PRIVATE include)
target_compile_options(${bench_executable_name} PRIVATE ${compile_options})
target_link_libraries(
  ${bench_executable_name}
  benchmark::benchmark
  glm::glm
  sfml-graphics
)

enable_testing()
include(GoogleTest)
gtest_discover_tests(${test_executable_name})