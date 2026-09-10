#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <vector>
namespace Aced::Test::Serialization {

    inline std::vector<std::byte> make_stream(std::initializer_list<std::uint8_t> content = {}) {
        std::vector<std::byte> input{
            std::byte{0xAC}, std::byte{0xED},
            std::byte{0x00}, std::byte{0x05}};

        for (const auto value : content) {
            input.push_back(static_cast<std::byte>(value));
        }
        return input;
    }

}