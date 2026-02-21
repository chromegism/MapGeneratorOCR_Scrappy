cmake_minimum_required(VERSION 3.27)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(MapGeneratorOCR_Scrappy CXX)

include(FetchContent)

find_package(Vulkan REQUIRED)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.2
)

FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.8.21
)
FetchContent_MakeAvailable(SDL3 tinygltf)

set(SOURCES 
src/main.cpp
    src/MapGen.cpp
    src/MapSettings.cpp
    src/Perlin.cpp
    src/Render.cpp
    src/Terrain.cpp
    src/Window.cpp
    src/App.cpp
)

add_executable(Main
    ${SOURCES}
)

target_link_libraries(Main PRIVATE
    Vulkan::Vulkan
    SDL3::SDL3
    tinygltf
)

target_include_directories(tinygltf
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

target_include_directories(Main PRIVATE ${PATH})