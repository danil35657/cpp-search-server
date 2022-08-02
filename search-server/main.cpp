#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <cmath>
#include <algorithm>
#include <utility>
#include <cstdlib>
#include <optional>
#include <stdexcept>
#include <numeric>
#include <execution>
#include <string_view>

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"
#include "test_example_functions.h"
#include "log_duration.h"
#include "remove_duplicates.h"
#include "process_queries.h"

using std::literals::string_literals::operator""s;

using std::literals::string_view_literals::operator""sv;

// ==================== для примера =========================

void PrintDocument(const Document& document) {
    std::cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string_view>& words, DocumentStatus status) {
    std::cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const std::string_view word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const std::exception& e) {
        std::cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    LOG_DURATION_STREAM("Operation time"s, std::cout);
    std::cout << "Результаты поиска по запросу: "s << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const std::exception& e) {
        std::cout << "Ошибка поиска: "s << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    LOG_DURATION_STREAM("Operation time"s, std::cout);
    try {
        std::cout << "Матчинг документов по запросу: "s << query << std::endl;
        for (const int document_id : search_server) {
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const std::exception& e) {
        std::cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << std::endl;
    }
}
    

int main() {
    // тесты
    TestSearchServer();
    
    // проверка основных методов SearchServer
    SearchServer search_server("и в на"s);

    AddDocument(search_server,  1, "пушистый кот пушистый хвост"s,     DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server,  1, "пушистый пёс и модный ошейник"s,   DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, -1, "пушистый пёс и модный ошейник"s,   DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server,  3, "большой пёс скво\x12рец евгений"s, DocumentStatus::ACTUAL, {1, 3, 2});
    AddDocument(search_server,  4, "большой пёс скворец евгений"s,     DocumentStatus::ACTUAL, {1, 1, 1});

    FindTopDocuments(search_server, "пушистый -пёс"s);
    FindTopDocuments(search_server, "пушистый --кот"s);
    FindTopDocuments(search_server, "пушистый -"s);

    MatchDocuments(search_server, "пушистый пёс"s);
    MatchDocuments(search_server, "модный -кот"s);
    MatchDocuments(search_server, "модный --пёс"s);
    MatchDocuments(search_server, "пушистый - хвост"s);
    
    // проверка вывода страницами
    std::cout << std::endl;
    AddDocument(search_server, 2, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, {1, 2, 3});
    AddDocument(search_server, 3, "большой кот модный ошейник "s,   DocumentStatus::ACTUAL, {1, 2, 8});
    AddDocument(search_server, 5, "большой пёс скворец василий"s,   DocumentStatus::ACTUAL, {1, 1, 1});
    
    const auto search_results = search_server.FindTopDocuments("пушистый пёс"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);

    for (auto page = pages.begin(); page != pages.end(); ++page) {
        std::cout << *page << std::endl;
        std::cout << "Разрыв страницы"s << std::endl;
    }

    //проверка очереди запросов
    std::cout << std::endl;
    RequestQueue request_queue(search_server);
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("пустой запрос"s);
    }
    
    request_queue.AddFindRequest("пушистый пёс"s);
    request_queue.AddFindRequest("большой ошейник"s);
    request_queue.AddFindRequest("скворец"s);
    std::cout << "Всего пустых запросов: "s << request_queue.GetNoResultRequests() << std::endl;
    
    //проверка удаления дубликатов с помощью RemoveDuplicates 
    std::cout << std::endl;
    AddDocument(search_server, 6, "пушистый кот пушистый пушистый хвост"s,   DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server, 7, "большой кот модный модный ошейник "s,   DocumentStatus::ACTUAL, {1, 2, 8});
    AddDocument(search_server, 8, "большой пёс скворец василий"s,   DocumentStatus::ACTUAL, {1, 1, 1});
    std::cout << "До удаления дубликатов: "s << search_server.GetDocumentCount() << std::endl;
    RemoveDuplicates(search_server);
    std::cout << "После удаления дубликатов: "s << search_server.GetDocumentCount() << std::endl;
    
    //проверка параллельных запросов 
    std::cout << std::endl;
    {
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const std::string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const std::vector<std::string> queries = {
        "nasty rat -not"s,
        "not very funny nasty pet"s,
        "curly hair"s
    };
    id = 0;
    for (
        const auto& documents : ProcessQueries(search_server, queries)
    ) {
        std::cout << documents.size() << " documents for query ["s << queries[id++] << "]"s << std::endl;
    }
    }
    //проверка параллельного и последовательного удаления документов 
    {
    std::cout << std::endl;
    
    SearchServer search_server("and with"s);

    int id = 0;
    for (
        const std::string& text : {
            "funny pet and nasty rat"s,
            "funny pet with curly hair"s,
            "funny pet and not very nasty rat"s,
            "pet with rat and rat and rat"s,
            "nasty rat with curly hair"s,
        }
    ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
    }

    const std::string query = "curly and funny"s;

    auto report = [&search_server, &query] {
        std::cout << search_server.GetDocumentCount() << " documents total, "s
            << search_server.FindTopDocuments(query).size() << " documents for query ["s << query << "]"s << std::endl;
    };

    report();
    // однопоточная версия
    search_server.RemoveDocument(5);
    report();
    // однопоточная версия
    search_server.RemoveDocument(std::execution::seq, 1);
    report();
    // многопоточная версия
    search_server.RemoveDocument(std::execution::par, 2);
    report();
    }
}  