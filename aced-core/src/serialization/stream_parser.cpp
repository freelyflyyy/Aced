#include "aced/serialization/stream_document.h"
#include "stream_constants.h"
#include <aced/serialization/stream_parser.h>

#include <aced/byte_reader.h>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <utility>
#include <vector>

namespace Aced::Serialization {

    class StreamParser final {

    public:
        StreamParser(std::vector<std::byte> input, ParseOptions options)
            : document_(std::move(input)),
              reader_(document_.source()),
              options_(options) {
        }

        [[nodiscard]] ParseResult run() {
            if (document_.source().size() > options_.max_input_bytes) {
                return fail(
                    ParseStatus::Failed,
                    ParseErrorCode::InputLimitExceeded,
                    0
                );
            }

            if (reader_.remaining() <
                sizeof(STREAM_MAGIC) + sizeof(STREAM_VERSION)) {
                return fail(
                    ParseStatus::Failed,
                    ParseErrorCode::TruncatedHeader,
                    document_.source().size()
                );
            }

            if (reader_.read_u16_be() != STREAM_MAGIC) {
                return fail(
                    ParseStatus::Failed,
                    ParseErrorCode::InvalidMagic,
                    0
                );
            }

            if (reader_.read_u16_be() != STREAM_VERSION) {
                return fail(
                    ParseStatus::Failed,
                    ParseErrorCode::UnsupportedVersion,
                    sizeof(STREAM_MAGIC)
                );
            }

            while (!reader_.empty()) {
                const auto offset = reader_.position();

                if (document_.records_.size() >= options_.max_records) {
                    return fail(
                        ParseStatus::Partial,
                        ParseErrorCode::RecordLimitExceeded,
                        offset
                    );
                }

                const auto token = reader_.read_u8();
                RecordKind kind = RecordKind::Null;
                switch (token) {
                case TC_NULL:
                    break;

                case TC_BLOCKDATA:
                case TC_BLOCKDATALONG:
                    if (const auto error = read_block_data(token)) {
                        return fail(
                            ParseStatus::Partial,
                            error->code,
                            error->offset
                        );
                    }
                    kind = RecordKind::BlockData;
                    break;

                default:
                    return fail(
                        ParseStatus::Partial,
                        ParseErrorCode::UnsupportedToken,
                        offset
                    );
                }
                document_.records_.push_back({kind, offset, reader_.position() - offset});
            }
            return {
                std::move(document_),
                ParseStatus::Complete,
                std::nullopt};
        }

    private:
        [[nodiscard]] std::optional<ParseError> read_block_data(std::uint8_t token) {
            const auto length_offset = reader_.position();

            const auto length_size =
                token == TC_BLOCKDATA
                    ? sizeof(std::uint8_t)
                    : sizeof(std::int32_t);

            if (reader_.remaining() < length_size) {
                return ParseError{
                    ParseErrorCode::TruncatedContent,
                    document_.source().size()};
            }

            const std::uint32_t length =
                token == TC_BLOCKDATA
                    ? reader_.read_u8()
                    : reader_.read_u32_be();

            if (length > std::numeric_limits<std::int32_t>::max()) {
                return ParseError{
                    ParseErrorCode::InvalidLength,
                    length_offset};
            }

            if (length > reader_.remaining()) {
                return ParseError{
                    ParseErrorCode::TruncatedContent,
                    document_.source().size()};
            }

            reader_.skip(length);
            return std::nullopt;
        }

        [[nodiscard]] ParseResult fail(
            ParseStatus status,
            ParseErrorCode code,
            std::size_t offset
        ) {
            return {
                std::move(document_),
                status,
                ParseError{code, offset}};
        }

        StreamDocument document_;
        ByteReader reader_;
        ParseOptions options_;
    };

    ParseResult parse_stream(
        std::vector<std::byte> input,
        ParseOptions options
    ) {
        return StreamParser{std::move(input), options}.run();
    }
}