#include "read_input_functions.h"

using std::literals::string_literals::operator""s;

std::string ReadLine() {
    std::string s;
    getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    std::cin >> result;
    ReadLine();
    return result;
}


std::ostream& operator<<(std::ostream& output, Document document) {
    output << "{ "s
    << "document_id = "s << document.id << ", "s
    << "relevance = "s << document.relevance << ", "s
    << "rating = "s << document.rating << " }"s;
    return output;
}