cmake_minimum_required(VERSION 3.20)
project(Axiom VERSION 1.0 LANGUAGES CXX)

set(SPIRV_CROSS_STATIC ON CACHE BOOL "" FORCE)
set(SPIRV_CROSS_SHARED OFF CACHE BOOL "" FORCE)
set(SPIRV_CROSS_CLI OFF CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Vendor/SpirvCross ${CMAKE_BINARY_DIR}/Vendor/SpirvCross)

set(SHADERC_SKIP_TESTS ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_EXAMPLES ON CACHE BOOL "" FORCE)
set(SHADERC_SKIP_COPYRIGHT_CHECK ON CACHE BOOL "" FORCE)
set(SHADERC_ENABLE_SHARED_CRT ON CACHE BOOL "" FORCE)

if (NOT EXISTS Vendor/Shaderc/third_party/spirv-tools/CMakeLists.txt)
  message(STATUS "[VENDOR] Syncing Shaderc dependencies...")
  execute_process(COMMAND python3
    ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Shaderc/utils/git-sync-deps
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Shaderc
    RESULT_VARIABLE SYNC_RESULT
  )
  if (NOT SYNC_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to sync Shaderc dependencies")
  endif()
endif()

add_subdirectory(
  ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/Shaderc
  ${CMAKE_BINARY_DIR}/Vendor/Shaderc
)
add_subdirectory(
  ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/AxImageLoader
  ${CMAKE_BINARY_DIR}/Vendor/AxImageLoader
)
add_subdirectory(
  ${CMAKE_CURRENT_SOURCE_DIR}/Vendor/AxModelLoader
  ${CMAKE_BINARY_DIR}/Vendor/AxModelLoader
)

file(GLOB_RECURSE ALL_SOURCES "Source/*.cpp" "Source/*.mm")
file(GLOB_RECURSE ALL_HEADERS "Source/*.h" "Source/*.hpp")
list(FILTER ALL_SOURCES EXCLUDE REGEX "Source/Platform/.*")
list(FILTER ALL_SOURCES EXCLUDE REGEX "Source/Renderer/Backend/.*")

if (WIN32)
  file(GLOB_RECURSE PLATFORM_SOURCES "Source/Platform/Windows/*.cpp")
  set(PLATFORM_DEFINE AX_PLATFORM_WINDOWS)
elseif(APPLE)
  file(GLOB_RECURSE PLATFORM_SOURCES "Source/Platform/MacOS/*.cpp")
  file(GLOB_RECURSE OBJCXX_SOURCES "Source/Platform/MacOS/*.mm")
  set_source_files_properties(${OBJCXX_SOURCES} PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
  list(APPEND PLATFORM_SOURCES ${OBJCXX_SOURCES})
  set(PLATFORM_DEFINE "AX_PLATFORM_MACOS")
elseif(UNIX AND NOT APPLE)
  file(GLOB_RECURSE PLATFORM_SOURCES "Source/Platform/Linux/*.cpp")
  set(PLATFORM_DEFINE "AX_PLATFORM_LINUX")
endif()
list(APPEND ALL_SOURCES ${PLATFORM_SOURCES})

if (AX_RENDERER STREQUAL "vulkan")
  file(GLOB_RECURSE RENDERER_SOURCES "Source/Renderer/Backend/Vulkan/*.cpp")
elseif (AX_RENDERER STREQUAL "metal")
  file(GLOB_RECURSE RENDERER_SOURCES "Source/Renderer/Backend/Metal/*.cpp")
elseif (AX_RENDERER STREQUAL "dx12")
  file(GLOB_RECURSE RENDERER_SOURCES "Source/Renderer/Backend/DX12/*.cpp")
endif()
list(APPEND ALL_SOURCES ${RENDERER_SOURCES})

add_library(Axiom STATIC ${ALL_SOURCES} ${ALL_HEADERS})


if (AX_RENDERER STREQUAL "vulkan")
  find_package(Vulkan REQUIRED)
  target_link_libraries(Axiom PUBLIC Vulkan::Vulkan)
elseif (AX_RENDERER STREQUAL "metal")
  if (APPLE)
    find_library(METAL_FRAMEWORK Metal)
    find_library(FOUNDATION_FRAMEWORK Foundation)
    find_library(QUARTZ_FRAMEWORK QuartzCore)
    find_library(APPKIT_FRAMEWORK AppKit)
    target_link_libraries(Axiom PUBLIC
      ${METAL_FRAMEWORK}
      ${FOUNDATION_FRAMEWORK}
      ${QUARTZ_FRAMEWORK}
      ${APPKIT_FRAMEWORK}
    )
  else()
    message(FATAL_ERROR "Metal renderer is only supported on Apple platforms")
  endif()
elseif (AX_RENDERER STREQUAL "dx12")
  if (WIN32)
    target_link_libraries(Axiom PUBLIC d3d12.lib dxgi.lib dxguid.lib)
  else()
    message(FATAL_ERROR "DirectX 12 renderer is only supported on Windows platforms")
endif()

endif()
target_compile_definitions(Axiom PUBLIC ${PLATFORM_DEFINE})
target_compile_definitions(Axiom PRIVATE AX_ROOT_DIR=\"${CMAKE_SOURCE_DIR}\")
target_include_directories(
  Axiom PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Source>
  Vendor/Vulkan/Include
  Vendor
  Vendor/AxImageLoader/Include
  Vendor/AxModelLoader/Include
  Vendor/SpirvCross
  Vendor/Shaderc/libshaderc/include
)
target_link_libraries(
  Axiom PUBLIC
  AxImageLoader
  AxModelLoader
  spirv-cross-core
  spirv-cross-glsl
  spirv-cross-msl
  shaderc_combined
)

target_precompile_headers(Axiom PRIVATE Source/axpch.h)
