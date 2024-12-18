cmake_minimum_required(VERSION 3.30)
project("3D_From_Scratch")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(program_executable_name ${CMAKE_PROJECT_NAME})
set(test_executable_name ${CMAKE_PROJECT_NAME}_TEST)
set(bench_executable_name ${CMAKE_PROJECT_NAME}_BENCH)

file(GLOB_RECURSE src_files CONFIGURE_DEPENDS src/*.cpp)

set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")
set(compile_options
  "$<${gcc_like_cxx}:-pedantic-errors;-Wall;-Weffc++;-Wextra;-Wsign-conversion;-Wconversion>"
  "$<${msvc_cxx}:/permissive-;/W4;/WX>"
)

include(FetchContent)

FetchContent_Declare(
  SFML
  GIT_REPOSITORY https://github.com/SFML/SFML.git
  GIT_TAG 2.5.1
)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
	glm
	GIT_REPOSITORY https://github.com/g-truc/glm.git
	GIT_TAG 45008b225e28eb700fa0f7d3ff69b7c1db94fadf
)
FetchContent_MakeAvailable(glm)

add_executable(${program_executable_name} ${src_files})
target_include_directories(${program_executable_name} PRIVATE include)
target_compile_options(${program_executable_name} PRIVATE ${compile_options})
target_link_libraries(
  ${program_executable_name}
  glm::glm
  sfml-graphics
)