#include "test_assertions.h"

#include <aced/serialization/stream_document.h>
#include <aced/serialization/stream_parser.h>

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <utility>
#include <vector>

using Aced::Serialization::parse_stream;
using Aced::Serialization::ParseErrorCode;
using Aced::Serialization::ParseOptions;
using Aced::Serialization::ParseStatus;
using Aced::Serialization::RecordKind;
using Aced::Test::require;

namespace {

    std::vector<std::byte> make_stream(std::initializer_list<std::uint8_t> content = {}) {
        std::vector<std::byte> input{
            std::byte{0xAC}, std::byte{0xED},
            std::byte{0x00}, std::byte{0x05}};

        for (const auto value : content) {
            input.push_back(static_cast<std::byte>(value));
        }

        return input;
    }

}

int main() {
    {
        const auto result = parse_stream(make_stream());

        require(result.status == ParseStatus::Complete);
        require(!result.error.has_value());
        require(result.document.records().empty());
        require(result.document.source().size() == 4);
    }

    {
        const auto result = parse_stream(make_stream({0x70, 0x70}));
        const auto records = result.document.records();

        require(result.status == ParseStatus::Complete);
        require(records.size() == 2);
        require(records[0].kind == RecordKind::Null);
        require(records[0].offset == 4);
        require(records[0].size == 1);
        require(records[1].offset == 5);
    }

    {
        for (std::size_t length = 0; length < 4; ++length) {
            auto input = make_stream();
            input.resize(length);

            const auto result = parse_stream(std::move(input));

            require(result.status == ParseStatus::Failed);
            require(result.document.records().empty());
            require(result.error.has_value());
            require(result.error->code == ParseErrorCode::TruncatedHeader);
            require(result.error->offset == length);
        }
    }

    {
        auto input = make_stream();
        input[0] = std::byte{0};

        const auto result = parse_stream(std::move(input));

        require(result.status == ParseStatus::Failed);
        require(result.error.has_value());
        require(result.error->code == ParseErrorCode::InvalidMagic);
        require(result.error->offset == 0);
    }

    {
        auto input = make_stream();
        input[3] = std::byte{6};

        const auto result = parse_stream(std::move(input));

        require(result.status == ParseStatus::Failed);
        require(result.error.has_value());
        require(result.error->code == ParseErrorCode::UnsupportedVersion);
        require(result.error->offset == 2);
    }

    {
        const auto result = parse_stream(make_stream({0x70, 0x74}));

        require(result.status == ParseStatus::Partial);
        require(result.document.records().size() == 1);
        require(result.error.has_value());
        require(result.error->code == ParseErrorCode::UnsupportedToken);
        require(result.error->offset == 5);
    }

    {
        const auto result = parse_stream(
            make_stream(),
            ParseOptions{.max_input_bytes = 3}
        );

        require(result.status == ParseStatus::Failed);
        require(result.error.has_value());
        require(result.error->code == ParseErrorCode::InputLimitExceeded);
    }

    {
        const auto result = parse_stream(
            make_stream({0x70}),
            ParseOptions{.max_records = 0}
        );

        require(result.status == ParseStatus::Partial);
        require(result.document.records().empty());
        require(result.error.has_value());
        require(result.error->code == ParseErrorCode::RecordLimitExceeded);
        require(result.error->offset == 4);
    }

    {
        const auto result = parse_stream(make_stream({
            0x77, 0x02,
            0x70, 0xFF,
            0x7A,
            0x00, 0x00, 0x00, 0x01,
            0x42,
            0x70
        }));
        const auto records = result.document.records();

        require(result.status == ParseStatus::Complete);
        require(!result.error);
        require(records.size() == 3);

        require(records[0].kind == RecordKind::BlockData);
        require(records[0].offset == 4);
        require(records[0].size == 4);

        require(records[1].kind == RecordKind::BlockData);
        require(records[1].offset == 8);
        require(records[1].size == 6);

        require(records[2].kind == RecordKind::Null);
        require(records[2].offset == 14);
    }

    {
        const auto result = parse_stream(make_stream({0x77, 0x00, 0x7A, 0x00, 0x00, 0x00, 0x00}));
        const auto records = result.document.records();

        require(result.status == ParseStatus::Complete);
        require(!result.error);
        require(records.size() == 2);
        require(records[0].kind == RecordKind::BlockData);
        require(records[0].offset == 4);
        require(records[0].size == 2);
        require(records[1].kind == RecordKind::BlockData);
        require(records[1].offset == 6);
        require(records[1].size == 5);
    }

    {
        struct FailureCase {
            std::vector<std::byte> input;
            ParseErrorCode code;
            std::size_t offset;
            std::size_t completed_records;
        };

        const FailureCase cases[]{
            {make_stream({0x77}), ParseErrorCode::TruncatedContent, 5, 0},
            {make_stream({0x7A, 0x00, 0x00}), ParseErrorCode::TruncatedContent, 7, 0},
            {make_stream({0x77, 0x02, 0xAA}), ParseErrorCode::TruncatedContent, 7, 0},
            {make_stream({0x70, 0x77, 0x02, 0xAA}), ParseErrorCode::TruncatedContent, 8, 1},
            {make_stream({0x7A, 0xFF, 0xFF, 0xFF, 0xFF}), ParseErrorCode::InvalidLength, 5, 0},
            {make_stream({0x7A, 0x80, 0x00, 0x00, 0x00}), ParseErrorCode::InvalidLength, 5, 0}};

        for (const auto& test : cases) {
            const auto result = parse_stream(test.input);

            require(result.status == ParseStatus::Partial);
            require(result.document.records().size() == test.completed_records);
            require(result.error.has_value());
            require(result.error->code == test.code);
            require(result.error->offset == test.offset);

            if (test.completed_records != 0) {
                require(result.document.records()[0].kind == RecordKind::Null);
                require(result.document.records()[0].offset == 4);
                require(result.document.records()[0].size == 1);
            }
        }
    }

    {
        const auto result = parse_stream(
            make_stream({0x77, 0x00, 0x70}),
            ParseOptions{.max_records = 1}
        );
        const auto records = result.document.records();

        require(result.status == ParseStatus::Partial);
        require(records.size() == 1);
        require(records[0].kind == RecordKind::BlockData);
        require(records[0].offset == 4);
        require(records[0].size == 2);
        require(result.error.has_value());
        require(result.error->code == ParseErrorCode::RecordLimitExceeded);
        require(result.error->offset == 6);
    }
}
