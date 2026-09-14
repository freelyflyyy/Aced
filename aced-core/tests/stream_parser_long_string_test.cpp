#include "serialization/stream_fixture.h"
#include "test_assertions.h"

#include <aced/serialization/stream_parser.h>

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

using Aced::Serialization::parse_stream;
using Aced::Serialization::ParseErrorCode;
using Aced::Serialization::ParseOptions;
using Aced::Serialization::ParseStatus;
using Aced::Serialization::RecordKind;
using Aced::Test::require;
using Aced::Test::Serialization::make_stream;

int main() {
    {
        // 65,536 encoded bytes exceed the short-string length range.
        auto input = make_stream({0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00});

        input.insert(input.end(), 65'536, std::byte{0x41});

        input.insert(input.end(), {std::byte{0x71}, std::byte{0x00}, std::byte{0x7E}, std::byte{0x00}, std::byte{0x00}});

        const auto result = parse_stream(std::move(input));
        const auto records = result.document.records();
        const auto nodes = result.document.nodes();

        require(result.status == ParseStatus::Complete);
        require(!result.error);
        require(records.size() == 2);
        require(nodes.size() == 1);

        require(records[0].kind == RecordKind::String);
        require(records[0].offset == 4);
        require(records[0].size == 65'545);
        require(records[0].node == 0);

        require(records[1].kind == RecordKind::Reference);
        require(records[1].offset == 65'549);
        require(records[1].size == 5);
        require(records[1].node == 0);

        require(
            std::get<std::u16string>(nodes[0]) ==
            std::u16string(65'536, u'A')
        );
    }

    {
        // The tag selects the length encoding, even for an empty payload.
        const auto result = parse_stream(
            make_stream({0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}),
            ParseOptions{.max_string_bytes = 0}
        );

        require(result.status == ParseStatus::Complete);
        require(result.document.records().size() == 1);
        require(result.document.records()[0].size == 9);
        require(result.document.nodes().size() == 1);
        require(
            std::get<std::u16string>(
                result.document.nodes()[0]
            )
                .empty()
        );
    }

    {
        // Exercise every incomplete eight-byte length prefix.
        for (std::size_t count = 0; count < 8; ++count) {
            auto input = make_stream({0x7C});
            input.resize(5 + count, std::byte{0});

            const auto result = parse_stream(std::move(input));

            require(result.status == ParseStatus::Partial);
            require(result.document.records().empty());
            require(result.document.nodes().empty());
            require(result.error.has_value());
            require(result.error->code == ParseErrorCode::TruncatedContent);
            require(result.error->offset == 5 + count);
        }
    }

    {
        struct FailureCase {
            std::vector<std::byte> input;
            ParseErrorCode code;
            std::size_t offset;
        };

        const FailureCase cases[]{
            {make_stream({0x7C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}),
             ParseErrorCode::InvalidLength, 5},
            {make_stream({0x7C, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}),
             ParseErrorCode::InvalidLength, 5},
            {// 2^32 must not narrow to zero before the limit check.
             make_stream({0x7C, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00}),
             ParseErrorCode::StringLimitExceeded, 5},
            {make_stream({0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x41}),
             ParseErrorCode::TruncatedContent, 14},
            {make_stream({0x7C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80}),
             ParseErrorCode::InvalidStringEncoding, 13}};

        for (const auto& test : cases) {
            const auto result = parse_stream(test.input);

            require(result.status == ParseStatus::Partial);
            require(result.document.records().empty());
            require(result.document.nodes().empty());
            require(result.error.has_value());
            require(result.error->code == test.code);
            require(result.error->offset == test.offset);
        }
    }
}