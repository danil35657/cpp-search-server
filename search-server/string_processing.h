#pragma once

#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include <algorithm>

using std::literals::string_literals::operator""s;

std::vector<std::string_view> SplitIntoWords(const std::string_view str);

bool IsValidWord(const std::string_view word);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string, std::less<>> non_empty_strings;
    for (const auto& str : strings) {
        if (!IsValidWord(str)) {
            throw std::invalid_argument("Некорректный ввод: "s + std::string(str));
        }
        if (!str.empty()) {
            non_empty_strings.insert(std::string(str));
        }
    }
    return non_empty_strings;
}