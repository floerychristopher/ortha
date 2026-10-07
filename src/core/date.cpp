#include "core/date.hpp"

#include <algorithm>
#include <charconv>
#include <format>

namespace ortha {

namespace {

std::string_view trim(std::string_view s) {
    const auto is_space = [](char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; };
    while (!s.empty() && is_space(s.front()))
        s.remove_prefix(1);
    while (!s.empty() && is_space(s.back()))
        s.remove_suffix(1);
    return s;
}

// Parses a field that must consist of min_len..max_len decimal digits only.
std::optional<int> parse_number(std::string_view s, std::size_t min_len, std::size_t max_len) {
    if (s.size() < min_len || s.size() > max_len)
        return std::nullopt;
    if (!std::all_of(s.begin(), s.end(), [](char c) { return c >= '0' && c <= '9'; }))
        return std::nullopt;
    int value = 0;
    std::from_chars(s.data(), s.data() + s.size(), value);
    return value;
}

std::optional<Date> make_date(std::optional<int> y, std::optional<int> m, std::optional<int> d) {
    if (!y || !m || !d)
        return std::nullopt;
    Date date{std::chrono::year{*y}, std::chrono::month{static_cast<unsigned>(*m)},
              std::chrono::day{static_cast<unsigned>(*d)}};
    if (!date.ok())
        return std::nullopt;
    return date;
}

} // namespace

std::optional<Date> parse_date(std::string_view text) {
    text = trim(text);
    const auto first = text.find('.');
    const auto second = text.find('.', first == std::string_view::npos ? first : first + 1);
    if (first == std::string_view::npos || second == std::string_view::npos)
        return std::nullopt;
    return make_date(parse_number(text.substr(second + 1), 4, 4),
                     parse_number(text.substr(first + 1, second - first - 1), 1, 2),
                     parse_number(text.substr(0, first), 1, 2));
}

std::string format_date(Date d) {
    return std::format("{:02}.{:02}.{:04}", static_cast<unsigned>(d.day()),
                       static_cast<unsigned>(d.month()), static_cast<int>(d.year()));
}

std::string to_iso(Date d) {
    return std::format("{:04}-{:02}-{:02}", static_cast<int>(d.year()),
                       static_cast<unsigned>(d.month()), static_cast<unsigned>(d.day()));
}

std::optional<Date> from_iso(std::string_view text) {
    if (text.size() != 10 || text[4] != '-' || text[7] != '-')
        return std::nullopt;
    return make_date(parse_number(text.substr(0, 4), 4, 4), parse_number(text.substr(5, 2), 2, 2),
                     parse_number(text.substr(8, 2), 2, 2));
}

Date today() {
    const auto local = std::chrono::current_zone()->to_local(std::chrono::system_clock::now());
    return Date{std::chrono::floor<std::chrono::days>(local)};
}

std::string now_utc_iso() {
    const auto now = std::chrono::floor<std::chrono::seconds>(std::chrono::system_clock::now());
    return std::format("{:%FT%TZ}", now);
}

} // namespace ortha
