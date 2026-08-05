cmake_minimum_required(VERSION 3.20)
project(ProjectAxiom VERSION 1.0 LANGUAGES C CXX)

set(AX_RENDERER "vulkan" CACHE STRING "Specify which renderer backend to use")
set_property(CACHE AX_RENDERER PROPERTY STRINGS "vulkan" "dx12" "metal")

set(AX_COMPILER "gcc" CACHE STRING "Specify which compiler should be used")
set_property(CACHE AX_COMPILER PROPERTY STRINGS "gcc" "g++" "msvc" "clang")

option(AX_VERBOSE "Enable verbose output" OFF)
option(AX_NO_COLOR "Disable colored output" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if (MSVC)
  add_compile_options(/std:c++latest)
endif()

if (MSVC)
  add_compile_options(/nologo /FS /permissive-)
  add_link_options(/nologo)
  add_compile_definitions(
    $<$<CONFIG:Debug>:AX_DEBUG>
    $<$<CONFIG:Debug>:AX_ENABLE_ASSERTS>
    $<$<CONFIG:Release>:AX_RELEASE>
  )
  else()
  add_compile_options(-pedantic -Wno-invalid-offsetof)
  add_compile_options(
    $<$<CONFIG:Release>:-march=native>
  )
  add_compile_definitions(
    $<$<CONFIG:Debug>:AX_DEBUG>
    $<$<CONFIG:Debug>:AX_ENABLE_ASSERTS>
    $<$<CONFIG:Release>:AX_RELEASE>
  )
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fexperimental-library)
  endif()
endif()

set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin/Runtime)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin/Shared)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin/Static)

add_subdirectory(Axiom)
add_subdirectory(Theorem)
