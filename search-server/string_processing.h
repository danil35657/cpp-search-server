#pragma once

#include <string>
#include <vector>
#include <set>
#include <stdexcept>
#include <algorithm>

using std::literals::string_literals::operator""s;

std::vector<std::string> SplitIntoWords(const std::string& text);

bool IsValidWord(const std::string& word);

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;
    for (const std::string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
        if (!IsValidWord(str)) {
            throw std::invalid_argument("Некорректный ввод: "s + str);
        }
    }
    return non_empty_strings;
}