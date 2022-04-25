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

using std::literals::string_literals::operator""s;

// ==================== ���� =========================

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file, const std::string& func, unsigned line, const std::string& hint) {
    if (t != u) {
        std::cerr << std::boolalpha;
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        std::cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line, const std::string& hint) {
    if (!value) {
        std::cerr << file << "("s << line << "): "s << func << ": "s;
        std::cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            std::cerr << " Hint: "s << hint;
        }
        std::cerr << std::endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, hint)

void TestAddAndFindDocument() {
    SearchServer server("� � ��"s);
    server.AddDocument(0, "���� ��� � ����� �襩���"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "������ ��� ������ 墮��"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    {
    std::vector<Document> v = server.FindTopDocuments("������"s);
    ASSERT_EQUAL(v.size(), 1);
    ASSERT_EQUAL(v[0].id, 1);
    ASSERT_EQUAL(v[0].rating, 5);
    }
    {
    std::vector<Document> v = server.FindTopDocuments("���"s);
    ASSERT_EQUAL(v.size(), 2);
    ASSERT_EQUAL(v[0].rating, 5);
    ASSERT_EQUAL(v[1].rating, 3);
    }
}

void TestExclusionOfStopWords() {
    SearchServer server("� � ��"s);
    server.AddDocument(0, "���� ��� � ����� �襩���"s,        DocumentStatus::ACTUAL, {8, -2});
    ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
}

void TestSupportForMinusWords() {
    SearchServer server("� � ��"s);
    server.AddDocument(0, "���� ��� � ����� �襩���"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "������ ��� ������ 墮��"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    std::vector<Document> v = server.FindTopDocuments("��� -����"s);
    ASSERT_EQUAL(v.size(), 1);
    ASSERT_EQUAL(v[0].id, 1);
    ASSERT_EQUAL(v[0].rating, 5);
}

void TestSortByRelevance() {
    SearchServer server("� � ��"s);
    server.AddDocument(0, "���� ��� � ����� �襩���"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "������ ��� ������ 墮��"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    std::vector<Document> v = server.FindTopDocuments("������ ���"s);
    ASSERT_EQUAL(v.size(), 2);
    ASSERT(v[1].relevance <= v[0].relevance);
    ASSERT_EQUAL(v[1].id, 0);
}

void TestComputeRating() {
    SearchServer server("� � ��"s);
    server.AddDocument(0, "���� ��� � ����� �襩���"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "������ ��� ������ 墮��"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    std::vector<Document> v = server.FindTopDocuments("���"s);
    ASSERT_EQUAL(v[0].rating, 5);
    ASSERT_EQUAL(v[1].rating, 3);
}

void TestFilteringByPredicate() {
    SearchServer server("� � ��"s);
    server.AddDocument(0, "���� ��� � ����� �襩���"s,       DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "������ ��� ������ 墮��"s,      DocumentStatus::ACTUAL, {7, 2, 6});
    std::vector<Document> v = server.FindTopDocuments("���"s, [](int, DocumentStatus, int rating) {
        return rating >= 4; });
    ASSERT_EQUAL(v.size(), 1);
    ASSERT_EQUAL(v[0].id, 1);
}

void TestSearchByStatus() {
    SearchServer server("� � ��"s);
    server.AddDocument(0, "���� ��� � ����� �襩���"s,       DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "������ ��� ������ 墮��"s,      DocumentStatus::ACTUAL, {7, 2, 6});
    server.AddDocument(2, "��� ��� � ����� �襩���"s,       DocumentStatus::BANNED, {8, -3});
    server.AddDocument(3, "���� ��� ��� 墮��"s,           DocumentStatus::BANNED, {7, 2, 7});
    server.AddDocument(4, "�宦���� ��� ��ࠧ�⥫�� �����"s,DocumentStatus::BANNED, {5, -12, 2, 1});
    server.AddDocument(5, "�宦���� ᪢��� �������"s,        DocumentStatus::IRRELEVANT, {9});
    server.AddDocument(6, "�宦���� ��� ��ࠧ�⥫�� �����"s,DocumentStatus::REMOVED, {5, -1, 2, 1});
    server.AddDocument(7, "�宦���� ᪢��� ��������"s,       DocumentStatus::REMOVED, {6});
    std::vector<Document> a = server.FindTopDocuments("���"s, DocumentStatus::ACTUAL);
    ASSERT_EQUAL(a.size(), 2);
    ASSERT_EQUAL(a[0].id, 1);
    std::vector<Document> b = server.FindTopDocuments("���"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(b.size(), 2);
    ASSERT_EQUAL(b[0].id, 4);
    std::vector<Document> i = server.FindTopDocuments("᪢���"s, DocumentStatus::IRRELEVANT);
    ASSERT_EQUAL(i.size(), 1);
    ASSERT_EQUAL(i[0].id, 5);
    std::vector<Document> r = server.FindTopDocuments("���"s, DocumentStatus::REMOVED);
    ASSERT_EQUAL(r.size(), 1);
    ASSERT_EQUAL(r[0].id, 6);
}

void TestCalculatingRelevance() {
    SearchServer server("� � ��"s);
    server.AddDocument(0, "���� ��� � ����� �襩���"s,       DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "������ ��� ������ 墮��"s,      DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "�宦���� ��� ��ࠧ�⥫�� �����"s,DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "�宦���� ᪢��� �������"s,        DocumentStatus::BANNED, {9});
    std::vector<Document> v = server.FindTopDocuments("������ �宦���� ���"s);
    ASSERT(std::abs(v[0].relevance - 0.866434) < EPSILON);
    ASSERT(std::abs(v[1].relevance - 0.173287) < EPSILON);
}

void TestSearchServer() {
    TestAddAndFindDocument();
    TestExclusionOfStopWords();
    TestSupportForMinusWords();
    TestSortByRelevance();
    TestComputeRating();
    TestFilteringByPredicate();
    TestSearchByStatus();
    TestCalculatingRelevance();
}

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

int main() { // ��� �஢�ન �᭮���� ��⮤��
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
}


/* int main() { // �஢�ઠ �뢮�� ��࠭�栬�
    SearchServer search_server("and with"s);

    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});

    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);

    for (auto page = pages.begin(); page != pages.end(); ++page) {
        std::cout << *page << std::endl;
        std::cout << "Page break"s << std::endl;
    }
} */
 
/* int main() { // �஢�ઠ ��।� ����ᮢ
    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    
    request_queue.AddFindRequest("curly dog"s);
    request_queue.AddFindRequest("big collar"s);
    request_queue.AddFindRequest("sparrow"s);
    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
    return 0;
}  */