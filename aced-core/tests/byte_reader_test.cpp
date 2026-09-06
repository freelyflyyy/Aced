#include "test_assertions.h"
#include <aced/byte_reader.h>

#include <array>
#include <limits>
#include <stdexcept>

using Aced::Test::require;
using Aced::Test::require_throws;

int main() {
    constexpr std::array data{
        std::byte{0xAC}, std::byte{0xED},
        std::byte{0x00}, std::byte{0x05}
    };

    Aced::ByteReader reader{data};

    require(reader.read_u16_be() == 0xACED);
    require(reader.position() == 2);

    require_throws<std::out_of_range>([&reader] {
        static_cast<void>(reader.read_u32_be());
    });

    require(reader.position() == 2);
    require(reader.remaining() == 2);

    require_throws<std::out_of_range>([&reader] {
        reader.skip(std::numeric_limits<std::size_t>::max());
    });
    require(reader.position() == 2);

    require(reader.read_u16_be() == 0x05);
    require(reader.empty());
    require(reader.read_bytes(0).empty());
    require_throws<std::out_of_range>([&reader] {
        static_cast<void>(reader.read_u8());
    });
    require(reader.position() == data.size());
    Aced::ByteReader full{data};
    require(full.read_u32_be() == 0xACED0005u);

    std::array<std::byte, 8> maximum;
    maximum.fill(std::byte{0xFF});

    Aced::ByteReader wide{maximum};
    require(
        wide.read_u64_be() == std::numeric_limits<std::uint64_t>::max()
    );

    Aced::ByteReader single{data};
    require(single.read_u8() == 0xAC);
    single.skip(2);
    require(single.read_bytes(1)[0] == std::byte{0x05});
    require(single.empty());
}
