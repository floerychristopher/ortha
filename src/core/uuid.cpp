#include "core/uuid.hpp"

#include <array>
#include <cstdint>
#include <random>

namespace ortha {

std::string make_uuid() {
    thread_local std::mt19937_64 engine{std::random_device{}()};

    std::array<std::uint8_t, 16> bytes{};
    for (auto& b : bytes)
        b = static_cast<std::uint8_t>(engine() & 0xff);
    bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0f) | 0x40); // version 4
    bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3f) | 0x80); // variant 10xx

    static constexpr char hex[] = "0123456789abcdef";
    std::string out;
    out.reserve(36);
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        if (i == 4 || i == 6 || i == 8 || i == 10)
            out += '-';
        out += hex[bytes[i] >> 4];
        out += hex[bytes[i] & 0x0f];
    }
    return out;
}

} // namespace ortha
