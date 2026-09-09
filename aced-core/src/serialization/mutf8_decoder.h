#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <variant>

namespace Aced::Serialization {

    struct Mutf8Error {
        std::size_t offset;
    };

    using Mutf8Result = std::variant<std::u16string, Mutf8Error>;

    [[nodiscard]] Mutf8Result decode_mutf8(std::span<const std::byte> input);
}