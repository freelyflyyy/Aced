#include <aced/serialization/stream_parser.h>

#include <aced/byte_reader.h>
#include <cstddef>
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

            if (reader_.remaining() < 4) {
                return fail(
                    ParseStatus::Failed,
                    ParseErrorCode::TruncatedHeader,
                    document_.source().size()
                );
            }

            if (reader_.read_u16_be() != 0xACED) {
                return fail(
                    ParseStatus::Failed,
                    ParseErrorCode::InvalidMagic,
                    0
                );
            }

            if (reader_.read_u16_be() != 5) {
                return fail(
                    ParseStatus::Failed,
                    ParseErrorCode::UnsupportedVersion,
                    2
                );
            }

            while (!reader_.empty()) {
                const auto offset = reader_.position();
                const auto token = reader_.read_u8();

                if (token != 0x70) {
                    return fail(
                        ParseStatus::Partial,
                        ParseErrorCode::UnsupportedToken,
                        offset
                    );
                }

                if (document_.records_.size() >= options_.max_records) {
                    return fail(
                        ParseStatus::Partial,
                        ParseErrorCode::RecordLimitExceeded,
                        offset
                    );
                }

                document_.records_.push_back({RecordKind::Null, offset, 1});
            }
            return {
                std::move(document_),
                ParseStatus::Complete,
                std::nullopt
            };
        }

    private:
        [[nodiscard]] ParseResult fail(
            ParseStatus status,
            ParseErrorCode code,
            std::size_t offset
        ) {
            return {
                std::move(document_),
                status,
                ParseError{code, offset}
            };
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