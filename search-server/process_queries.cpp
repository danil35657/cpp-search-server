#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<std::vector<Document>> result(queries.size());
    transform(std::execution::par, queries.begin(), queries.end(), result.begin(), [&search_server] (const std::string& s) {return search_server.FindTopDocuments(s); });
    return result;
}

std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
    std::vector<Document> result;
    for (const auto& a : ProcessQueries(search_server, queries)) {
        result.insert(result.end(), a.begin(), a.end());
    }
    return result;
}