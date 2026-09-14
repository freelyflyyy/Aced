add_library(aced-test-support INTERFACE)

target_include_directories(aced-test-support INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/support"
)

target_compile_features(aced-test-support INTERFACE cxx_std_20)

function(aced_add_test name)
    set(target "aced-${name}-test")

    add_executable(${target} ${ARGN})

    target_link_libraries(${target} PRIVATE
        aced::core
        aced-test-support
    )

    add_test(
        NAME "aced.${name}"
        COMMAND ${target}
    )
endfunction(aced_add_test name)

aced_add_test(byte_reader byte_reader_test.cpp)
aced_add_test(stream_parser stream_parser_test.cpp)

aced_add_test(mutf8_decoder mutf8_decoder_test.cpp)

target_include_directories(aced-mutf8_decoder-test PRIVATE
    "${PROJECT_SOURCE_DIR}/src/serialization"
)

aced_add_test(stream_parser_reference
    stream_parser_reference_test.cpp
    serialization/stream_fixture.h
)

aced_add_test(stream_parser_long_string
    stream_parser_long_string_test.cpp
)
