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

#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"
#include "tests.h"

using std::literals::string_literals::operator""s;

// ==================== ��� �ਬ�� =========================

void PrintDocument(const Document& document) {
    std::cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << std::endl;
}

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status) {
    std::cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const std::string& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}"s << std::endl;
}

void AddDocument(SearchServer& search_server, int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const std::exception& e) {
        std::cout << "�訡�� ���������� ���㬥�� "s << document_id << ": "s << e.what() << std::endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const std::string& raw_query) {
    std::cout << "�������� ���᪠ �� ������: "s << raw_query << std::endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const std::exception& e) {
        std::cout << "�訡�� ���᪠: "s << e.what() << std::endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const std::string& query) {
    try {
        std::cout << "���稭� ���㬥�⮢ �� ������: "s << query << std::endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const std::exception& e) {
        std::cout << "�訡�� ���稭�� ���㬥�⮢ �� ����� "s << query << ": "s << e.what() << std::endl;
    }
}

int main() {
    //����
    TestSearchServer();
    
    // �஢�ઠ �᭮���� ��⮤�� SearchServer
    SearchServer search_server("� � ��"s);

    AddDocument(search_server,  1, "������ ��� ������ 墮��"s,     DocumentStatus::ACTUAL, {7, 2, 7});
    AddDocument(search_server,  1, "������ ��� � ����� �襩���"s,   DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server, -1, "������ ��� � ����� �襩���"s,   DocumentStatus::ACTUAL, {1, 2});
    AddDocument(search_server,  3, "����让 ��� ᪢�\x12�� �������"s, DocumentStatus::ACTUAL, {1, 3, 2});
    AddDocument(search_server,  4, "����让 ��� ᪢��� �������"s,     DocumentStatus::ACTUAL, {1, 1, 1});

    FindTopDocuments(search_server, "������ -���"s);
    FindTopDocuments(search_server, "������ --���"s);
    FindTopDocuments(search_server, "������ -"s);

    MatchDocuments(search_server, "������ ���"s);
    MatchDocuments(search_server, "����� -���"s);
    MatchDocuments(search_server, "����� --���"s);
    MatchDocuments(search_server, "������ - 墮��"s);
    
    // �஢�ઠ �뢮�� ��࠭�栬�
    std::cout << std::endl;
    search_server.AddDocument(2, "������ ��� � ����� �襩���"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "����让 ��� ����� �襩��� "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(5, "����让 ��� ᪢��� ��ᨫ��"s, DocumentStatus::ACTUAL, {1, 1, 1});
    
    const auto search_results = search_server.FindTopDocuments("������ ���"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);

    for (auto page = pages.begin(); page != pages.end(); ++page) {
        std::cout << *page << std::endl;
        std::cout << "����� ��࠭���"s << std::endl;
    }

    //�஢�ઠ ��।� ����ᮢ
    std::cout << std::endl;
    RequestQueue request_queue(search_server);
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("���⮩ �����"s);
    }
    
    request_queue.AddFindRequest("������ ���"s);
    request_queue.AddFindRequest("����让 �襩���"s);
    request_queue.AddFindRequest("᪢���"s);
    std::cout << "�ᥣ� ������ ����ᮢ: "s << request_queue.GetNoResultRequests() << std::endl;
    return 0;
} 