#include "core/task.hpp"

#include <algorithm>

namespace ortha {

std::vector<std::string> parse_tags(std::string_view input) {
    std::vector<std::string> tags;
    std::size_t start = 0;
    while (start <= input.size()) {
        auto end = input.find(',', start);
        if (end == std::string_view::npos)
            end = input.size();

        auto part = input.substr(start, end - start);
        while (!part.empty() && (part.front() == ' ' || part.front() == '\t'))
            part.remove_prefix(1);
        while (!part.empty() && (part.back() == ' ' || part.back() == '\t'))
            part.remove_suffix(1);

        std::string tag;
        for (char c : part)
            tag += (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;
        if (!tag.empty())
            tags.push_back(std::move(tag));
        start = end + 1;
    }
    std::sort(tags.begin(), tags.end());
    tags.erase(std::unique(tags.begin(), tags.end()), tags.end());
    return tags;
}

std::string join_tags(const std::vector<std::string>& tags) {
    std::string out;
    for (const auto& tag : tags) {
        if (!out.empty())
            out += ", ";
        out += tag;
    }
    return out;
}

} // namespace ortha
