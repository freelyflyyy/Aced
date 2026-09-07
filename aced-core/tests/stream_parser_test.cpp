#include "test_assertions.h"

#include <aced/serialization/stream_parser.h>

using Aced::Test::require;
using namespace Aced::Serialization;

namespace {

    std::vector<std::byte> make_stream() {
        return {
            std::byte{0xAC}, std::byte{0xED},
            std::byte{0x00}, std::byte{0x05}
        };
    }

}

int main() {
    {
        auto result = parse_stream(make_stream());

        require(result.status == ParseStatus::Complete);
        require(!result.error);
        require(result.document.records().empty());
        require(result.document.source().size() == 4);
    }

    {
        auto input = make_stream();
        input.push_back(std::byte{0x70});
        input.push_back(std::byte{0x70});

        auto result = parse_stream(std::move(input));
        const auto records = result.document.records();

        require(result.status == ParseStatus::Complete);
        require(records.size() == 2);
        require(records[0].kind == RecordKind::Null);
        require(records[0].offset == 4);
        require(records[0].size == 1);
        require(records[1].offset == 5);
    }

    {
        auto result = parse_stream({});

        require(result.status == ParseStatus::Failed);
        require(result.error.has_value());
        require(result.error->code == ParseErrorCode::TruncatedHeader);
        require(result.error->offset == 0);
    }

    {
        auto input = make_stream();
        input[0] = std::byte{0};

        auto result = parse_stream(std::move(input));

        require(result.status == ParseStatus::Failed);
        require(result.error.has_value());
        require(result.error->code == ParseErrorCode::InvalidMagic);
        require(result.error->offset == 0);
    }

    {
        auto input = make_stream();
        input[3] = std::byte{6};

        auto result = parse_stream(std::move(input));

        require(result.status == ParseStatus::Failed);
        require(result.error.has_value());
        require(result.error->code == ParseErrorCode::UnsupportedVersion);
        require(result.error->offset == 2);
    }

    {
        auto input = make_stream();
        input.push_back(std::byte{0x70});
        input.push_back(std::byte{0x74});

        auto result = parse_stream(std::move(input));

        require(result.status == ParseStatus::Partial);
        require(result.document.records().size() == 1);
        require(result.error.has_value());
        require(result.error->code == ParseErrorCode::UnsupportedToken);
        require(result.error->offset == 5);
    }

    {
        auto result = parse_stream(
            make_stream(),
            ParseOptions{.max_input_bytes = 3}
        );

        require(result.status == ParseStatus::Failed);
        require(result.error.has_value());
        require(result.error->code == ParseErrorCode::InputLimitExceeded);
    }

    {
        auto input = make_stream();
        input.push_back(std::byte{0x70});

        auto result = parse_stream(
            std::move(input),
            ParseOptions{.max_records = 0}
        );

        require(result.status == ParseStatus::Partial);
        require(result.document.records().empty());
        require(result.error.has_value());
        require(result.error->code == ParseErrorCode::RecordLimitExceeded);
        require(result.error->offset == 4);
    }
}