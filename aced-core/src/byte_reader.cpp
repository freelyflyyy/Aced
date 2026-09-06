#include <aced/byte_reader.h>

#include <stdexcept>

namespace Aced {

    ByteReader::ByteReader(std::span<const std::byte> data) noexcept
        : data_(data) {
    }

    std::size_t ByteReader::position() const noexcept {
        return position_;
    }

    std::size_t ByteReader::remaining() const noexcept {
        return data_.size() - position_;
    }

    bool ByteReader::empty() const noexcept {
        return remaining() == 0;
    }

    void ByteReader::advance(std::size_t count) {
        if (count > remaining()) {
            throw std::out_of_range("ByteReader: insufficient data");
        }

        position_ += count;
    }

    std::span<const std::byte> ByteReader::read_bytes(std::size_t count) {
        const auto current_pos = position_;
        advance(count);
        return data_.subspan(current_pos, count);
    }

    void ByteReader::skip(std::size_t count) {
        advance(count);
    }

    template <std::unsigned_integral T>
    T ByteReader::read_be() {
        const auto bytes = read_bytes(sizeof(T));
        T value = 0;

        for (const auto byte : bytes) {
            value = static_cast<T>(
                (value << 8) | std::to_integer<unsigned int>(byte)
            );
        }

        return value;
    }

    std::uint8_t ByteReader::read_u8() {
        return read_be<std::uint8_t>();
    }

    std::uint16_t ByteReader::read_u16_be() {
        return read_be<std::uint16_t>();
    }

    std::uint32_t ByteReader::read_u32_be() {
        return read_be<std::uint32_t>();
    }

    std::uint64_t ByteReader::read_u64_be() {
        return read_be<std::uint64_t>();
    }

} // namespace aced