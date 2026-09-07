#pragma once

#include <cstddef>
#include <span>
#include <vector>
namespace Aced::Serialization {
    class StreamParser;
    enum class RecordKind {
        Null
    };

    struct StreamRecord {
        RecordKind kind;
        std::size_t offset;
        std::size_t size;
    };

    class StreamDocument final {
    public:
        [[nodiscard]] std::span<const std::byte> source() const noexcept {
            return source_;
        }

        [[nodiscard]] std::span<const StreamRecord> records() const noexcept {
            return records_;
        }

    private:
        friend class StreamParser;

        explicit StreamDocument(std::vector<std::byte> source)
            : source_(std::move(source)) {
        }

        std::vector<std::byte> source_;
        std::vector<StreamRecord> records_;
    };
}