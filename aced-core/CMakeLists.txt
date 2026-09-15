cmake_minimum_required (VERSION 3.21)

# Project
# =======

project(
    AcedCore
    VERSION 0.0.1
    LANGUAGES CXX
)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Source files
# ============

set(ACED_BYTE_READER_FILES
        include/aced/byte_reader.h
        src/byte_reader.cpp
)

set(ACED_SERIALIZATION_FILES
        include/aced/serialization/stream_document.h
        include/aced/serialization/stream_parser.h
        src/serialization/stream_constants.h
        src/serialization/stream_parser.cpp
        src/serialization/mutf8_decoder.h
        src/serialization/mutf8_decoder.cpp
)

# CMake utilities
# ===============

function(aced_add_test name)
    cmake_parse_arguments(PARSE_ARGV 1 TEST
            ""
            ""
            "SOURCES;LIBRARIES;INCLUDE_DIRS"
    )

    if (TEST_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
                "aced_add_test(${name}): unknown arguments: "
                "${TEST_UNPARSED_ARGUMENTS}"
        )
    endif ()

    if (TEST_KEYWORDS_MISSING_VALUES)
        message(FATAL_ERROR
                "aced_add_test(${name}): missing values for: "
                "${TEST_KEYWORDS_MISSING_VALUES}"
        )
    endif ()

    if (NOT TEST_SOURCES)
        message(FATAL_ERROR
                "aced_add_test(${name}): SOURCES is required"
        )
    endif ()

    set(target "aced-${name}-test")

    add_executable(${target} ${TEST_SOURCES})

    target_link_libraries(${target} PRIVATE
            aced-test-support
            ${TEST_LIBRARIES}
    )

    if(TEST_INCLUDE_DIRS)
        target_include_directories(${target} PRIVATE
                ${TEST_INCLUDE_DIRS}
        )
    endif()

    source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}
            FILES ${TEST_SOURCES}
    )

    add_test(
            NAME "aced.${name}"
            COMMAND ${target}
    )
endfunction()

# Core library
# ============

add_library(aced-core STATIC
        ${ACED_BYTE_READER_FILES}
        ${ACED_SERIALIZATION_FILES}
)

add_library(aced::core ALIAS aced-core)

target_compile_features(aced-core PUBLIC cxx_std_20)

target_include_directories(aced-core PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES
            ${ACED_BYTE_READER_FILES}
            ${ACED_SERIALIZATION_FILES}
)

# Tests
# =====

include(CTest)

if (BUILD_TESTING)
    # Shared test support
    # -------------------

    add_library(aced-test-support INTERFACE)

    target_compile_features(aced-test-support INTERFACE
            cxx_std_20
    )

    target_include_directories(aced-test-support INTERFACE
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/support"
    )

    # Byte reader
    # -----------

    aced_add_test(byte_reader
            SOURCES
            tests/byte_reader_test.cpp
            LIBRARIES
            aced::core
    )

    # Serialization
    # -------------

    aced_add_test(mutf8_decoder
            SOURCES
            tests/mutf8_decoder_test.cpp
            LIBRARIES
            aced::core
            INCLUDE_DIRS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/serialization"
    )

    aced_add_test(stream_parser
            SOURCES
            tests/stream_parser_test.cpp
            LIBRARIES
            aced::core
    )

    aced_add_test(stream_parser_reference
            SOURCES
            tests/stream_parser_reference_test.cpp
            tests/serialization/stream_fixture.h
            LIBRARIES
            aced::core
    )

    aced_add_test(stream_parser_long_string
            SOURCES
            tests/stream_parser_long_string_test.cpp
            LIBRARIES
            aced::core
    )
endif ()
