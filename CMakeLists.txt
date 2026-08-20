cmake_minimum_required(VERSION 3.20)
project(Theorem VERSION 1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_COLOR_DIAGNOSTICS ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin/Runtime)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin/Shared)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Bin/Static)

add_subdirectory(Engine/Axiom)

file(GLOB_RECURSE THEOREM_SOURCES "Source/*.cpp")
file(GLOB_RECURSE THEOREM_HEADERS "Source/*.h")

add_executable(Theorem ${THEOREM_SOURCES} ${THEOREM_HEADERS})
target_link_libraries(Theorem PRIVATE Axiom)
target_include_directories(Theorem PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Source)

target_compile_definitions(Theorem PRIVATE
  $<$<CONFIG:Debug>:TH_DEBUG>
  $<$<CONFIG:Release>:TH_RELEASE>
)

if (MSVC)
  target_compile_options(Theorem PRIVATE /nologo /FS /permissive-)
  target_link_options(Theorem PRIVATE /nologo)
else()
  target_compile_options(Theorem PRIVATE -pedantic -Wno-invalid-offsetof)
  target_compile_options(Theorem PRIVATE $<$<CONFIG:Release>:-march=native>)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(Theorem PRIVATE -fexperimental-library)
  endif()
endif()
