#include "mutf8_decoder.h"
#include <cstddef>
#include <cstdint>
#include <string>

namespace Aced::Serialization {

    Mutf8Result decode_mutf8(std::span<const std::byte> input) {
        std::u16string output;
        std::size_t position = 0;

        while (position < input.size()) {
            const auto first = std::to_integer<std::uint8_t>(input[position]);

            // 0xxxxxxxx: a byte
            if ((first & 0b1000'0000) == 0) {
                output.push_back(static_cast<char16_t>(first));
                position++;
                continue;
            }

            std::size_t width;
            std::uint32_t value;

            // 110xxxxx: two byte
            if ((first & 0b1110'0000) == 0b1100'0000) {
                width = 2;
                value = first & 0b0001'1111;
            }
            // 1110xxxx: three byte
            else if ((first & 0b1111'0000) == 0b1110'0000) {
                width = 3;
                value = first & 0b0000'1111;
            } else {
                return Mutf8Error{position};
            }

            if (input.size() - position < width) {
                return Mutf8Error{input.size()};
            }

            for (std::size_t index = 1; index < width; ++index) {
                const auto next = std::to_integer<std::uint8_t>(input[position + index]);

                // Subsequent bytes are required 10xxxxxx
                if ((next & 0b1100'0000) != 0b1000'0000) {
                    return Mutf8Error{position + index};
                }

                value = (value << 6) | (next & 0b0011'1111);
            }

            output.push_back(static_cast<char16_t>(value));
            position += width;
        }

        return output;
    }
}