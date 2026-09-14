#pragma once

#include <cstdint>
namespace Aced::Serialization {

    inline constexpr std::uint16_t STREAM_MAGIC = 0XACED;
    inline constexpr std::uint16_t STREAM_VERSION = 5;

    inline constexpr std::uint8_t TC_NULL = 0x70;
    inline constexpr std::uint8_t TC_REFERENCE = 0x71;
    inline constexpr std::uint8_t TC_STRING = 0x74;
    inline constexpr std::uint8_t TC_BLOCKDATA = 0x77;
    inline constexpr std::uint8_t TC_RESET = 0x79;
    inline constexpr std::uint8_t TC_BLOCKDATALONG = 0x7A;
    inline constexpr std::uint8_t TC_LONGSTRING = 0x7C;

    inline constexpr std::uint32_t BASE_WIRE_HANDLE = 0x7E0000;
}