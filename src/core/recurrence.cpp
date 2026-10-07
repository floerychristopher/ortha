#include "core/recurrence.hpp"

#include <algorithm>
#include <array>
#include <charconv>

namespace ortha {

namespace ch = std::chrono;

namespace {

// Index = ISO weekday - 1 (Monday = 0).
constexpr std::array<std::string_view, 7> kDayCodes = {"MO", "TU", "WE", "TH", "FR", "SA", "SU"};
constexpr std::array<std::string_view, 4> kFreqNames = {"DAILY", "WEEKLY", "MONTHLY", "YEARLY"};

Date add_months_clamped(Date d, ch::months m) {
    const auto ym = ch::year_month{d.year(), d.month()} + m;
    const auto last_day = ch::year_month_day_last{ym.year(), ch::month_day_last{ym.month()}}.day();
    return Date{ym.year(), ym.month(), std::min(d.day(), last_day)};
}

Date next_weekly(const RecurrenceRule& rule, Date base) {
    const ch::sys_days day{base};
    if (rule.weekdays.empty())
        return Date{day + ch::weeks{rule.interval}};

    const unsigned today = ch::weekday{day}.iso_encoding(); // 1 = Monday .. 7 = Sunday
    unsigned later_this_week = 8;
    unsigned first_of_week = 8;
    for (auto wd : rule.weekdays) {
        const unsigned iso = wd.iso_encoding();
        first_of_week = std::min(first_of_week, iso);
        if (iso > today)
            later_this_week = std::min(later_this_week, iso);
    }
    if (later_this_week != 8)
        return Date{day + ch::days{later_this_week - today}};

    const auto monday = day - ch::days{today - 1};
    return Date{monday + ch::weeks{rule.interval} + ch::days{first_of_week - 1}};
}

std::vector<std::string_view> split(std::string_view text, char sep) {
    std::vector<std::string_view> parts;
    std::size_t start = 0;
    while (true) {
        const auto end = text.find(sep, start);
        parts.push_back(text.substr(start, end - start));
        if (end == std::string_view::npos)
            return parts;
        start = end + 1;
    }
}

} // namespace

std::string serialize(const RecurrenceRule& rule) {
    std::string out = "FREQ=";
    out += kFreqNames[static_cast<std::size_t>(rule.freq)];
    out += ";INTERVAL=" + std::to_string(rule.interval);
    if (!rule.weekdays.empty()) {
        auto days = rule.weekdays;
        std::sort(days.begin(), days.end(),
                  [](auto a, auto b) { return a.iso_encoding() < b.iso_encoding(); });
        out += ";BYDAY=";
        for (std::size_t i = 0; i < days.size(); ++i) {
            if (i > 0)
                out += ',';
            out += kDayCodes[days[i].iso_encoding() - 1];
        }
    }
    out += rule.anchor == Anchor::Due ? ";FROM=DUE" : ";FROM=COMPLETION";
    return out;
}

std::optional<RecurrenceRule> parse_recurrence(std::string_view text) {
    RecurrenceRule rule;
    bool has_freq = false;
    for (auto part : split(text, ';')) {
        const auto eq = part.find('=');
        if (eq == std::string_view::npos)
            return std::nullopt;
        const auto key = part.substr(0, eq);
        const auto value = part.substr(eq + 1);

        if (key == "FREQ") {
            const auto it = std::find(kFreqNames.begin(), kFreqNames.end(), value);
            if (it == kFreqNames.end())
                return std::nullopt;
            rule.freq = static_cast<Frequency>(it - kFreqNames.begin());
            has_freq = true;
        } else if (key == "INTERVAL") {
            const auto [ptr, ec] =
                std::from_chars(value.data(), value.data() + value.size(), rule.interval);
            if (ec != std::errc{} || ptr != value.data() + value.size() || rule.interval < 1)
                return std::nullopt;
        } else if (key == "BYDAY") {
            for (auto code : split(value, ',')) {
                const auto it = std::find(kDayCodes.begin(), kDayCodes.end(), code);
                if (it == kDayCodes.end())
                    return std::nullopt;
                // weekday{n} uses 0 = Sunday, so ISO 7 (Sunday) maps to 0 via the modulo.
                rule.weekdays.push_back(
                    ch::weekday{static_cast<unsigned>((it - kDayCodes.begin() + 1) % 7)});
            }
        } else if (key == "FROM") {
            if (value == "DUE")
                rule.anchor = Anchor::Due;
            else if (value == "COMPLETION")
                rule.anchor = Anchor::Completion;
            else
                return std::nullopt;
        } else {
            return std::nullopt;
        }
    }
    if (!has_freq || (!rule.weekdays.empty() && rule.freq != Frequency::Weekly))
        return std::nullopt;

    std::sort(rule.weekdays.begin(), rule.weekdays.end(),
              [](auto a, auto b) { return a.iso_encoding() < b.iso_encoding(); });
    rule.weekdays.erase(std::unique(rule.weekdays.begin(), rule.weekdays.end()),
                        rule.weekdays.end());
    return rule;
}

Date next_due(const RecurrenceRule& rule, Date base) {
    switch (rule.freq) {
    case Frequency::Daily:
        return Date{ch::sys_days{base} + ch::days{rule.interval}};
    case Frequency::Weekly:
        return next_weekly(rule, base);
    case Frequency::Monthly:
        return add_months_clamped(base, ch::months{rule.interval});
    case Frequency::Yearly:
        return add_months_clamped(base, ch::months{12 * rule.interval});
    }
    return base;
}

} // namespace ortha
