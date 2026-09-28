cmake_minimum_required(VERSION 3.28)
project(my_hud LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/lib")

# Common compile definitions for Windows
if (WIN32)
    add_compile_definitions(
        WIN32_LEAN_AND_MEAN
        NOMINMAX
        UNICODE
        _UNICODE
    )
endif()

add_subdirectory(thirdparty/imgui-docking)
add_subdirectory(thirdparty/implot)
add_subdirectory(thirdparty/minhook)

# Define a common "library" for ImGui setup to avoid code duplication
# This includes ImGui, ImPlot, and backends used by BOTH targets
add_library(GuiCommon INTERFACE)
target_include_directories(GuiCommon INTERFACE 
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui-docking"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/implot"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)
target_link_libraries(GuiCommon INTERFACE imgui implot)


#############################################
# Target 1: The Control Panel (Standalone EXE)
#############################################

file(GLOB_RECURSE EXE_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/exe/*.cpp")

add_executable(my_hud_panel ${EXE_SOURCES})

target_sources(my_hud_panel PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui-docking/imgui/backends/imgui_impl_win32.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui-docking/imgui/backends/imgui_impl_dx9.cpp"
)

# Control Panel needs Win32 backend for ImGui
target_link_libraries(my_hud_panel PRIVATE 
    GuiCommon 
    d3d9 
    dwmapi
)

target_compile_definitions(my_hud_panel PRIVATE IS_CONTROL_PANEL)


#############################################
# Target 2: The Overlay (Injected DLL)
#############################################

file(GLOB_RECURSE DLL_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/dll/*.cpp")

# Define as SHARED library (creates .dll and .lib)
add_library(my_hud_dll SHARED ${DLL_SOURCES})

# Rename output from my_hud_dll.dll to simply my_hud.dll
set_target_properties(my_hud_dll PROPERTIES 
    PREFIX "" 
    OUTPUT_NAME "my_hud"
)

target_sources(my_hud_dll PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui-docking/imgui/backends/imgui_impl_win32.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/imgui-docking/imgui/backends/imgui_impl_dx11.cpp"
)

target_link_libraries(my_hud_dll PRIVATE 
    GuiCommon
    minhook
    d3d11
    dxgi
    dwmapi
    d3dcompiler
)

target_compile_definitions(my_hud_dll PRIVATE IS_OVERLAY_DLL)

#if(NOT EXISTS "${CMAKE_SOURCE_DIR}/src/shared/SharedMemoryInterface/InternalsPlugin.hpp")
#    message(FATAL_ERROR "Missing LMU SDK headers. See README: copy them from the game folder into src/shared/SharedMemoryInterface/")
#endif()