cmake_minimum_required(VERSION 3.27)
project(NBody)

include(FetchContent)

FetchContent_Declare(
    NBodyExecutor
    GIT_REPOSITORY https://github.com/stripe2933/NBodyExecutor.git
    GIT_TAG 7962f88
)
FetchContent_MakeAvailable(NBodyExecutor)

FetchContent_Declare(
    OpenGLApp
    GIT_REPOSITORY https://github.com/stripe2933/OpenGLApp.git
    GIT_TAG 7de1780
)
FetchContent_MakeAvailable(OpenGLApp)

find_package(imgui REQUIRED)
find_package(fmt REQUIRED)
find_package(range-v3 REQUIRED)

add_executable(NBody main.cpp
        src/App.cpp
        src/SimulationView.cpp
        src/BodyPreset.cpp
        src/SimulationGridView.cpp
        src/Dialogs/NewSimulationViewDialog.cpp
        src/NaiveSimulationView.cpp
        src/BarnesHutSimulationView.cpp
        src/Dialogs/NewSimulationDataDialog.cpp
        src/SimulationData.cpp
        src/NaiveSimulationData.cpp
        include/NaiveSimulationData.hpp
        src/BarnesHutSimulationData.cpp
        include/BarnesHutSimulationData.hpp
)
target_compile_features(NBody PRIVATE cxx_std_20)
target_include_directories(NBody PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include ${CMAKE_CURRENT_SOURCE_DIR}/extlibs)
target_link_libraries(NBody PRIVATE NBodyExecutor OpenGLApp imgui::imgui fmt::fmt range-v3::range-v3)

# Copy shader files to executable folder.
add_custom_target(copy_shaders COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_LIST_DIR}/copy_shaders.cmake)
add_dependencies(${PROJECT_NAME} copy_shaders)