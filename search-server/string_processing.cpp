#include "string_processing.h"

using std::literals::string_literals::operator""s;


std::vector<std::string_view> SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;
    str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));
    
    while (str.size() != 0) {
        auto space = str.find(" ");
        result.push_back(space == str.npos ? str : str.substr(0, space));
        str.remove_prefix(std::min(space, str.size()));
        str.remove_prefix(std::min((str.find_first_not_of(" ")), str.size()));
    }
    return result;
}


bool IsValidWord(const std::string_view word) {
    return std::none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}
