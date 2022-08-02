#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> duplicate_numbers;
    std::set<std::set<std::string>> set_without_duplicates;
    
    for (const int document_id : search_server) {
        std::set<std::string> check_set;
        const auto& check_document = search_server.GetWordFrequencies(document_id);
        for (const auto word : check_document) {
            check_set.emplace(word.first);
        }
        if (set_without_duplicates.count(check_set) == 0) {
            set_without_duplicates.emplace(check_set);
        } else {
            duplicate_numbers.emplace(document_id);
        }
    }
    
    for (const int duplicate_number : duplicate_numbers) {
        std::cout << "Found duplicate document id "s << duplicate_number << std::endl;
        search_server.RemoveDocument(duplicate_number);
    }
}

void RemoveDuplicates(std::execution::sequenced_policy policy, SearchServer& search_server) {
    return RemoveDuplicates(search_server);
}

void RemoveDuplicates(std::execution::parallel_policy policy, SearchServer& search_server) {
    std::set<int> duplicate_numbers;
    std::set<std::set<std::string>> set_without_duplicates;
    
    for (const int document_id : search_server) {
        std::set<std::string> check_set;
        const auto& check_document = search_server.GetWordFrequencies(document_id);
        for (const auto word : check_document) {
            check_set.emplace(word.first);
        }
        if (set_without_duplicates.count(check_set) == 0) {
            set_without_duplicates.emplace(check_set);
        } else {
            duplicate_numbers.emplace(document_id);
        }
    }
    
    for (const int duplicate_number : duplicate_numbers) {
        std::cout << "Found duplicate document id "s << duplicate_number << std::endl;
        search_server.RemoveDocument(policy, duplicate_number);
    }
}