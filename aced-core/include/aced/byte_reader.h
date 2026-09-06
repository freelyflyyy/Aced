#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Aced {

    class ByteReader final {
    public:
        explicit ByteReader(std::span<const std::byte> data) noexcept;

        [[nodiscard]] std::size_t position() const noexcept;
        [[nodiscard]] std::size_t remaining() const noexcept;
        [[nodiscard]] bool empty() const noexcept;

        [[nodiscard]] std::span<const std::byte> read_bytes(std::size_t count);
        void skip(std::size_t count);

        [[nodiscard]] std::uint8_t read_u8();
        [[nodiscard]] std::uint16_t read_u16_be();
        [[nodiscard]] std::uint32_t read_u32_be();
        [[nodiscard]] std::uint64_t read_u64_be();

    private:
        void advance(std::size_t count);

        template <std::unsigned_integral T>
        [[nodiscard]] T read_be();

        std::span<const std::byte> data_;
        std::size_t position_ = 0;
    };

} // namespace aced