cmake_minimum_required(VERSION 3.15)

project(MatsuokaEngine LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "" FORCE)
endif()

file(GLOB MAT_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/*.c"
)

add_library(MatsuokaEngine STATIC ${MAT_SOURCES})

target_include_directories(MatsuokaEngine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(MatsuokaEngine PRIVATE DEBUG)
endif()
