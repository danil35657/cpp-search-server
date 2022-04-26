#include "tests.h"

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
    ASSERT_EQUAL(b[0].id, 2);
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