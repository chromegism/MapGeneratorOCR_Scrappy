cmake_minimum_required(VERSION 3.27)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project(MapGeneratorOCR_Scrappy CXX)

include(FetchContent)

find_package(Vulkan REQUIRED)

message(STATUS "Fetching SDL3 from https://github.com/libsdl-org/SDL.git")
FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.4.2
)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)

message(STATUS "Fetching tinygltf from https://github.com/syoyo/tinygltf.git")
FetchContent_Declare(
    tinygltf
    GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
    GIT_TAG v2.9.7
)
message(STATUS "Fetching glm from https://github.com/icaven/glm.git")
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 0.9.9.8
)
FetchContent_MakeAvailable(SDL3 tinygltf glm)
target_compile_definitions(tinygltf INTERFACE
    JSON_USE_IMPLICIT_CONVERSIONS=0
)

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

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

add_executable(main
    ${SOURCES}
)

target_link_libraries(main PRIVATE
    Vulkan::Vulkan
    SDL3::SDL3
    tinygltf
    glm::glm
)

target_include_directories(tinygltf
    INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
        $<INSTALL_INTERFACE:include>
)

set(CMAKE_CXX_FLAGS_RELEASE "-O3")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Configuring for debug build")
    target_compile_definitions(main PRIVATE _DEBUG)
else()
    message(STATUS "Configuring for release build")
endif()

file(GLOB_RECURSE SHADER_FILES
     ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/*)

add_custom_target(copy_shaders ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders
            $<TARGET_FILE_DIR:main>/shaders
    COMMENT "Copying shaders"
)

add_dependencies(main copy_shaders)