cmake_minimum_required(VERSION 3.10)
project(implot LANGUAGES CXX)

file(GLOB implot_sources CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)

add_library(implot STATIC ${implot_sources})

# Ensure headers are visible to anything that links to ImPlot
target_include_directories(implot
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link with ImGui
target_link_libraries(implot PUBLIC imgui)

# Optional: Make sure ImPlot appears in CMake GUI
set_target_properties(implot PROPERTIES FOLDER "thirdparty")