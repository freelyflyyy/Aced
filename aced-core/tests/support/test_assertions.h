#pragma once

#include <source_location>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Aced::Test {
    [[noreturn]] inline void fail(
        std::string_view message,
        std::source_location location = std::source_location::current()
    ) {
        throw std::runtime_error(
            std::string{location.file_name()} + ":" +
            std::to_string(location.line()) + ": " +
            std::string{message}
        );
    }

    inline void require(
        bool condition,
        std::source_location location = std::source_location::current()
    ) {
        if (!condition) {
            fail("requirement", location);
        }
    }

    template <typename Exception, typename Operation>
    void require_throws(
        Operation&& operation,
        std::source_location location = std::source_location::current()
    ) {
        try {
            operation();
        } catch (const Exception&) {
            return;
        } catch (...) {
            fail("Unexpected exception type", location);
        }

        fail("expected exception was not thrown", location);
    }

}
