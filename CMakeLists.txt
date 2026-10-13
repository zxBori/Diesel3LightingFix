cmake_minimum_required(VERSION 3.28)

# Disable C++ modules to speed up the build
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

# This is the name of the resulting library, without the .dll
set(plugin_name Diesel3LightingFix)

# This is the project name, often shown in IDEs
project(Diesel3LightingFix)

# Use CXX 20
set(CMAKE_CXX_STANDARD 20)

# Find the source file
file(GLOB_RECURSE sources src/*.cpp include/*.h)

# Add the library
add_library(${plugin_name} SHARED ${sources})

add_subdirectory(minhook)

target_include_directories(${plugin_name} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/minhook/include
)

target_link_libraries(${plugin_name} PRIVATE
    minhook
    d3d11
    dxgi
)

# Don't include the bloated bits of the Windows SDK
target_compile_definitions(${plugin_name} PUBLIC WIN32_LEAN_AND_MEAN)

# Add the SDK files
target_include_directories(${plugin_name} PUBLIC sdk)
target_sources(${plugin_name} PRIVATE sdk/setup.cpp sdk/plugin_sdk_import.gen.cpp)

# For development purposes, you may find this helpful
# The mod the DLL will be copied into a mod folder whenever
# you compile the project

# eg mymod copies it to mods/mymod/<the plugin name>.dll
set(target_mod_name mymod)

# Uncooment this to enable it
#add_custom_command(
#	TARGET ${plugin_name}
#	POST_BUILD
#	COMMAND powershell -command copy \\\"$<TARGET_FILE:${plugin_name}>\\\" \\\"C:/Program Files \(x86\)/Steam/steamapps/common/PAYDAY 2/mods/${target_mod_name}/${plugin_name}.dll\\\"
#)

