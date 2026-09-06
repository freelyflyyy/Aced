cmake_minimum_required (VERSION 3.21)

project(
    AcedCore
    VERSION 0.0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(ACED_HEADERS
    "include/aced/byte_reader.h"
    "include/aced/serialization/stream_document.h"
    "include/aced/serialization/stream_parser.h"
)

set(ACED_SOURCES
    "src/byte_reader.cpp"
    "src/serialization/stream_parser.cpp"
)

add_library(aced-core STATIC
    ${ACED_HEADERS}
    ${ACED_SOURCES}
)

add_library(aced::core ALIAS aced-core)

target_compile_features(aced-core PUBLIC cxx_std_20)

target_include_directories(aced-core
    PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include")

include(CTest)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
