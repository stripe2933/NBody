cmake_minimum_required(VERSION 3.27)
project(NBody)

include(FetchContent)

FetchContent_Declare(
    NBodyExecutor
    GIT_REPOSITORY https://github.com/stripe2933/NBodyExecutor.git
    GIT_TAG main
)
FetchContent_MakeAvailable(NBodyExecutor)

FetchContent_Declare(
    OpenGLApp
    GIT_REPOSITORY https://github.com/stripe2933/OpenGLApp.git
    GIT_TAG main
)
FetchContent_MakeAvailable(OpenGLApp)

find_package(imgui REQUIRED)
find_package(fmt REQUIRED)

set(CMAKE_CXX_STANDARD 20)

add_executable(NBody main.cpp
        src/App.cpp
        include/App.hpp
        src/SimulationView.cpp
        include/SimulationView.hpp
        src/BodyPreset.cpp
        include/BodyPreset.hpp)
target_include_directories(NBody PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(NBody PRIVATE NBodyExecutor OpenGLApp imgui::imgui fmt::fmt)

# Copy shader files to executable folder.
add_custom_target(copy_shaders COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/copy_shaders.cmake)
add_dependencies(${PROJECT_NAME} copy_shaders)