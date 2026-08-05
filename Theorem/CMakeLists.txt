cmake_minimum_required(VERSION 3.20)
project(Theorem VERSION 1.0 LANGUAGES CXX)

file(GLOB_RECURSE THEOREM_SOURCES "Source/*.cpp" "Source/*.h")

add_executable(Theorem ${THEOREM_SOURCES})

target_link_libraries(Theorem PRIVATE Axiom)
target_precompile_headers(Theorem REUSE_FROM Axiom)
