#include "mutf8_decoder.h"
#include "test_assertions.h"
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <variant>
#include <vector>

using Aced::Serialization::decode_mutf8;
using Aced::Serialization::Mutf8Error;
using Aced::Test::require;

namespace {

    std::vector<std::byte> make_data(std::initializer_list<std::uint8_t> values) {
        std::vector<std::byte> data;

        for (const auto value : values) {
            data.push_back(static_cast<std::byte>(value));
        }
        return data;
    }
}

int main() {
    {
        struct SuccessCase {
            std::vector<std::byte> input;
            std::u16string expected;
        };

        // clang-format off
        const SuccessCase cases[]{
            {make_data({}), u""},
            {make_data({0x41}), u"A"},
            {make_data({0xC0,0x80}), std::u16string{u'\0'}},
            {make_data({0xC2, 0x80}), u"\u0080"},
            {make_data({0xDF, 0xBF}), u"\u07FF"},
            {make_data({0xE4, 0xB8, 0xAD}), u"\u4E2D"},
            {make_data({0xEF, 0xBF, 0xBF}), u"\uFFFF"},
            {
                make_data({0xED, 0xA0, 0xBD, 0xED, 0xB8, 0x80}),
                u"\U0001F600"
            },
            {
                make_data({0xED, 0xA0, 0x80}),
                std::u16string(1, static_cast<char16_t>(0xD800))
            },
            // readUTF 的兼容读取行为。
            {make_data({0x00}), std::u16string(1, u'\0')},
            {make_data({0xC1, 0x81}), u"A"},
            {make_data({0xE0, 0x81, 0x81}), u"A"}
        };
        // clang-format on

        for (const auto& test : cases) {
            const auto result = decode_mutf8(test.input);
            const auto* text = std::get_if<std::u16string>(&result);

            require(text != nullptr);
            require(*text == test.expected);
        }
    }

    {
        struct FailureCase {
            std::vector<std::byte> input;
            std::size_t offset;
        };

        // clang-format off
        const FailureCase cases[]{
            {make_data({0x80}), 0},
            {make_data({0xFF}), 0},
            {make_data({0x41, 0x80}), 1},
            {make_data({0xC2}), 1},
            {make_data({0xE4, 0xB8}), 2},
            {make_data({0xC2, 0x41}), 1},
            {make_data({0xE4, 0x41, 0x80}), 1},
            {make_data({0xF0, 0x9F, 0x98, 0x80}), 0}
        };
        // clang-format on

        for (const auto& test : cases) {
            const auto result = decode_mutf8(test.input);
            const auto* error = std::get_if<Mutf8Error>(&result);

            require(error != nullptr);
            require(error->offset == test.offset);
        }
    }
}