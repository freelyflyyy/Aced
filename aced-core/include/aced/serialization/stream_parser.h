#pragma once

#include "stream_document.h"

#include <optional>

namespace Aced::Serialization {

    enum class ParseStatus {
        Complete,
        Partial,
        Failed
    };

    enum class ParseErrorCode {
        TruncatedHeader,
        InvalidMagic,
        UnsupportedVersion,
        UnsupportedToken,
        InputLimitExceeded,
        RecordLimitExceeded
    };

    struct ParseError {
        ParseErrorCode code;
        std::size_t offset;
    };

    struct ParseOptions {
        std::size_t max_input_bytes = 16 * 1024 * 1024;
        std::size_t max_records = 1'000'000;
    };

    struct ParseResult {
        StreamDocument document;
        ParseStatus status;
        std::optional<ParseError> error;
    };

    [[nodiscard]] ParseResult parse_stream(
        std::vector<std::byte> input,
        ParseOptions options = {}
    );
}