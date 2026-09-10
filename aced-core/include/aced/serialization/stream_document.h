#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <variant>
#include <vector>
namespace Aced::Serialization {
    class StreamParser;

    using NodeId = std::size_t;
    using StreamNode = std::variant<std::u16string>;

    enum class RecordKind {
        Null,
        BlockData,
        String
    };

    struct StreamRecord {
        RecordKind kind;
        std::size_t offset;
        std::size_t size;
        std::optional<NodeId> node = std::nullopt;
    };

    class StreamDocument final {
    public:
        [[nodiscard]] std::span<const std::byte> source() const noexcept {
            return source_;
        }

        [[nodiscard]] std::span<const StreamRecord> records() const noexcept {
            return records_;
        }

        [[nodiscard]] std::span<const StreamNode> nodes() const noexcept {
            return nodes_;
        }

    private:
        friend class StreamParser;

        explicit StreamDocument(std::vector<std::byte> source)
            : source_(std::move(source)) {
        }

        std::vector<std::byte> source_;
        std::vector<StreamRecord> records_;
        std::vector<StreamNode> nodes_;
    };
}
