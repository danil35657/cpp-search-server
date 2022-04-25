#pragma once

#include <string>
#include <vector>
#include <queue>

#include "document.h"
#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server);

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
        std::vector<Document> v = search_server_.FindTopDocuments(raw_query, document_predicate);
        QueryResult query;
        query.size = v.size();
        if (requests_.size() == min_in_day_) {
            requests_.pop_front();
        }
        requests_.push_back(query);
        return v;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;
    
private:
    struct QueryResult {
        size_t size;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
}; 