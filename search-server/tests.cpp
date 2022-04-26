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
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    {
    std::vector<Document> v = server.FindTopDocuments("пушистый"s);
    ASSERT_EQUAL(v.size(), 1);
    ASSERT_EQUAL(v[0].id, 1);
    ASSERT_EQUAL(v[0].rating, 5);
    }
    {
    std::vector<Document> v = server.FindTopDocuments("кот"s);
    ASSERT_EQUAL(v.size(), 2);
    ASSERT_EQUAL(v[0].rating, 5);
    ASSERT_EQUAL(v[1].rating, 3);
    }
}

void TestExclusionOfStopWords() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -2});
    ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
}

void TestSupportForMinusWords() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    std::vector<Document> v = server.FindTopDocuments("кот -белый"s);
    ASSERT_EQUAL(v.size(), 1);
    ASSERT_EQUAL(v[0].id, 1);
    ASSERT_EQUAL(v[0].rating, 5);
}

void TestSortByRelevance() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    std::vector<Document> v = server.FindTopDocuments("пушистый кот"s);
    ASSERT_EQUAL(v.size(), 2);
    ASSERT(v[1].relevance <= v[0].relevance);
    ASSERT_EQUAL(v[1].id, 0);
}

void TestComputeRating() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    std::vector<Document> v = server.FindTopDocuments("кот"s);
    ASSERT_EQUAL(v[0].rating, 5);
    ASSERT_EQUAL(v[1].rating, 3);
}

void TestFilteringByPredicate() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,       DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,      DocumentStatus::ACTUAL, {7, 2, 6});
    std::vector<Document> v = server.FindTopDocuments("кот"s, [](int, DocumentStatus, int rating) {
        return rating >= 4; });
    ASSERT_EQUAL(v.size(), 1);
    ASSERT_EQUAL(v[0].id, 1);
}

void TestSearchByStatus() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,       DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,      DocumentStatus::ACTUAL, {7, 2, 6});
    server.AddDocument(2, "серый кот и модный ошейник"s,       DocumentStatus::BANNED, {8, -3});
    server.AddDocument(3, "белый пес черный хвост"s,           DocumentStatus::BANNED, {7, 2, 7});
    server.AddDocument(4, "ухоженный кот выразительные глаза"s,DocumentStatus::BANNED, {5, -12, 2, 1});
    server.AddDocument(5, "ухоженный скворец евгений"s,        DocumentStatus::IRRELEVANT, {9});
    server.AddDocument(6, "ухоженный пес выразительные глаза"s,DocumentStatus::REMOVED, {5, -1, 2, 1});
    server.AddDocument(7, "ухоженный скворец геннадий"s,       DocumentStatus::REMOVED, {6});
    std::vector<Document> a = server.FindTopDocuments("кот"s, DocumentStatus::ACTUAL);
    ASSERT_EQUAL(a.size(), 2);
    ASSERT_EQUAL(a[0].id, 1);
    std::vector<Document> b = server.FindTopDocuments("кот"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(b.size(), 2);
    ASSERT_EQUAL(b[0].id, 2);
    std::vector<Document> i = server.FindTopDocuments("скворец"s, DocumentStatus::IRRELEVANT);
    ASSERT_EQUAL(i.size(), 1);
    ASSERT_EQUAL(i[0].id, 5);
    std::vector<Document> r = server.FindTopDocuments("пес"s, DocumentStatus::REMOVED);
    ASSERT_EQUAL(r.size(), 1);
    ASSERT_EQUAL(r[0].id, 6);
}

void TestCalculatingRelevance() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,       DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,      DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s,DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,        DocumentStatus::BANNED, {9});
    std::vector<Document> v = server.FindTopDocuments("пушистый ухоженный кот"s);
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