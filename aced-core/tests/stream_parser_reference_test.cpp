#include "aced/serialization/stream_document.h"
#include "aced/serialization/stream_parser.h"
#include "serialization/stream_fixture.h"
#include "test_assertions.h"
#include <cstddef>
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
        // clang-format off
        const auto result = parse_stream(
            make_stream({
                0x74, 0x00, 0x01, 0x41,
                0x71,0x00, 0x07E, 0x00, 0x00,
                0x79,
                0x74, 0x00,0x01,0x42,
                0x71,0x00,0x7E,0x00,0x00
            }),
            ParseOptions{.max_handles = 1}
        );
        // clang-format on

        const auto records = result.document.records();
        const auto nodes = result.document.nodes();

        require(result.status == ParseStatus::Complete);
        require(!result.error);
        require(records.size() == 5);
        require(nodes.size() == 2);

        require(records[0].kind == RecordKind::String);
        require(records[0].node == 0);

        require(records[1].kind == RecordKind::Reference);
        require(records[1].node == 0);
        require(records[1].offset == 8);
        require(records[1].size == 5);

        require(records[2].kind == RecordKind::Reset);
        require(!records[2].node);
        require(records[2].offset == 13);
        require(records[2].size == 1);

        require(records[3].node == 1);
        require(records[4].kind == RecordKind::Reference);
        require(records[4].node == 1);

        require(std::get<std::u16string>(nodes[0]) == u"A");
        require(std::get<std::u16string>(nodes[1]) == u"B");
    }
    {
        // clang-format off
        const auto result = parse_stream(make_stream({
            0x70,
            0x77, 0x00,
            0x74,0x00,0x01,0x41,
            0x71,0x00,0x7E,0x00,0x00,
            0x74, 0x00, 0x01, 0x42,
            0x71, 0x00, 0x7E, 0x00, 0x01
        }));
        // clang-format on

        const auto records = result.document.records();

        require(result.status == ParseStatus::Complete);
        require(records.size() == 6);
        require(result.document.nodes().size() == 2);
        require(records[2].node == 0);
        require(records[3].node == 0);
        require(records[4].node == 1);
        require(records[5].node == 1);
    }
    {
        struct FailureCase {
            std::vector<std::byte> input;
            ParseErrorCode code;
            std::size_t offset;
            std::size_t records;
            std::size_t nodes;
        };

        // clang-format off
        const FailureCase cases[] {
            {
                make_stream({0x71, 0x00, 0x7E, 0x00, 0x00}),
                ParseErrorCode::InvalidReference, 5, 0, 0
            },
            {
                make_stream({0x71, 0x00, 0x7D, 0xFF,0xFF}),
                ParseErrorCode::InvalidReference, 5, 0, 0
            },
            {
                make_stream({0x71, 0x00, 0x7E, 0x00}),
                ParseErrorCode::TruncatedContent, 8, 0, 0
            },
            {
                make_stream({
                    0x74, 0x00, 0x01, 0x41,
                    0x71, 0x00, 0x7E, 0x00, 0x01
                }),
                ParseErrorCode::InvalidReference, 9, 1, 1
            },
            {
                make_stream({
                    0x74, 0x00, 0x01, 0x41,
                    0x79,
                    0x71, 0x00, 0x7E, 0x00, 0x00
                }),
                ParseErrorCode::InvalidReference, 10, 2, 1
            }
        };
        // clang-format on

        for (const auto& test : cases) {
            const auto result = parse_stream(test.input);

            require(result.status == ParseStatus::Partial);
            require(result.document.records().size() == test.records);
            require(result.document.nodes().size() == test.nodes);
            require(result.error.has_value());
            require(result.error->code == test.code);
            require(result.error->offset == test.offset);
        }
    }

    {
        const auto result = parse_stream(
            make_stream({0x74, 0x00, 0x00}),
            ParseOptions{.max_handles = 0}
        );

        require(result.status == ParseStatus::Partial);
        require(result.document.records().empty());
        require(result.document.nodes().empty());
        require(result.error.has_value());
        require(result.error->code == ParseErrorCode::HandleLimitExceeded);
        require(result.error->offset == 5);
    }

    {
        const auto result = parse_stream(make_stream({0x79, 0x79}));

        require(result.status == ParseStatus::Complete);
        require(result.document.records().size() == 2);
        require(result.document.nodes().empty());
        require(!result.document.records()[0].node);
        require(!result.document.records()[1].node);
    }
}
