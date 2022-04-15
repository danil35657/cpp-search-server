#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cstdlib>
#include <optional>
#include <stdexcept>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

struct Document {
    
    Document(int id = 0, double relevance = 0.0, int rating = 0) : id(id), relevance(relevance), rating(rating) {
    }

    int id;
    double relevance;
    int rating;
};

bool IsValidWord(const string& word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c >= '\0' && c < ' ';
    });
}

template <typename StringContainer>
set<string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    set<string> non_empty_strings;
    for (const string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
        if (!IsValidWord(str)) {
            throw invalid_argument("Некорректный ввод: "s + str);
        }
    }
    return non_empty_strings;
}

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:

    template <typename StringContainer>
    explicit SearchServer(const StringContainer& stop_words)
        : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
    }

    explicit SearchServer(const string& stop_words_text)
        : SearchServer(SplitIntoWords(stop_words_text)) {
    }

    void AddDocument (int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0) {
            throw invalid_argument("ID не может быть отрицательным"s);
        } else if (documents_.count(document_id)) {
            throw invalid_argument("документ с таким ID уже есть"s);
        } else if (!IsValidWord(document)) {
            throw invalid_argument("Некорректный ввод"s);
        }
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
        document_ids_.push_back(document_id);
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                return lhs.rating > rhs.rating;
            } else {
                return lhs.relevance > rhs.relevance;
            }
        });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }
        return tuple(matched_words, documents_.at(document_id).status);
    }

    int GetDocumentId(int index) const {
        if (index < 0 || index >= documents_.size()) {
            throw out_of_range("Документ с таким индексом отсутствует"s);
        }
        return document_ids_[index];
    }

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };
    const set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    vector<int> document_ids_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) {
            return 0;
        }
        int rating_sum = 0;
        for (const int rating : ratings) {
            rating_sum += rating;
        }
        return rating_sum / static_cast<int>(ratings.size());
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;
        for (char c : text) {
            if (c >= '\0' && c < ' ') {
                throw invalid_argument("Некорректный ввод"s);
            }
        }
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        if (text[0] == '-' || text.empty()) {
            throw invalid_argument("Некорректный ввод"s);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
            const QueryWord query_word = ParseQueryWord(word);
            if (!query_word.is_stop) {
                if (query_word.is_minus) {
                    query.minus_words.insert(query_word.data);
                } else {
                    query.plus_words.insert(query_word.data);
                }
            }
        }
        return query;
    }

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }

};

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file, const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line, const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, hint)

void TestAddAndFindDocument() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    {
    vector<Document> v = server.FindTopDocuments("пушистый"s);
    ASSERT_EQUAL(v.size(), 1);
    ASSERT_EQUAL(v[0].id, 1);
    ASSERT_EQUAL(v[0].rating, 5);
    }
    {
    vector<Document> v = server.FindTopDocuments("кот"s);
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
    vector<Document> v = server.FindTopDocuments("кот -белый"s);
    ASSERT_EQUAL(v.size(), 1);
    ASSERT_EQUAL(v[0].id, 1);
    ASSERT_EQUAL(v[0].rating, 5);
}

void TestSortByRelevance() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    vector<Document> v = server.FindTopDocuments("пушистый кот"s);
    ASSERT_EQUAL(v.size(), 2);
    ASSERT(v[1].relevance <= v[0].relevance);
    ASSERT_EQUAL(v[1].id, 0);
}

void TestComputeRating() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    vector<Document> v = server.FindTopDocuments("кот"s);
    ASSERT_EQUAL(v[0].rating, 5);
    ASSERT_EQUAL(v[1].rating, 3);
}

void TestFilteringByPredicate() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,       DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,      DocumentStatus::ACTUAL, {7, 2, 6});
    vector<Document> v = server.FindTopDocuments("кот"s, [](int, DocumentStatus, int rating) {
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
    vector<Document> a = server.FindTopDocuments("кот"s, DocumentStatus::ACTUAL);
    ASSERT_EQUAL(a.size(), 2);
    ASSERT_EQUAL(a[0].id, 1);
    vector<Document> b = server.FindTopDocuments("кот"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(b.size(), 2);
    ASSERT_EQUAL(b[0].id, 4);
    vector<Document> i = server.FindTopDocuments("скворец"s, DocumentStatus::IRRELEVANT);
    ASSERT_EQUAL(i.size(), 1);
    ASSERT_EQUAL(i[0].id, 5);
    vector<Document> r = server.FindTopDocuments("пес"s, DocumentStatus::REMOVED);
    ASSERT_EQUAL(r.size(), 1);
    ASSERT_EQUAL(r[0].id, 6);
}

void TestCalculatingRelevance() {
    SearchServer server("и в на"s);
    server.AddDocument(0, "белый кот и модный ошейник"s,       DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "пушистый кот пушистый хвост"s,      DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "ухоженный пёс выразительные глаза"s,DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "ухоженный скворец евгений"s,        DocumentStatus::BANNED, {9});
    vector<Document> v = server.FindTopDocuments("пушистый ухоженный кот"s);
    ASSERT(abs(v[0].relevance - 0.866434) < EPSILON);
    ASSERT(abs(v[1].relevance - 0.173287) < EPSILON);
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

// ==================== для примера =========================

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating << " }"s << endl;
}

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
    try {
        search_server.AddDocument(document_id, document, status, ratings);
    } catch (const exception& e) {
        cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "Результаты поиска по запросу: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const exception& e) {
        cout << "Ошибка поиска: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "Матчинг документов по запросу: "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const exception& e) {
        cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
    }
}

int main() {
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
}