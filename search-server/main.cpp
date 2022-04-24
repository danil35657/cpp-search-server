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
#include <numeric>
#include <queue>

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
            throw invalid_argument("�����४�� ����: "s + str);
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
            throw invalid_argument("ID �� ����� ���� ����⥫��"s);
        } else if (documents_.count(document_id)) {
            throw invalid_argument("���㬥�� � ⠪�� ID 㦥 ����"s);
        } else if (!IsValidWord(document)) {
            throw invalid_argument("�����४�� ����"s);
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
            throw out_of_range("���㬥�� � ⠪�� �����ᮬ ���������"s);
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
		return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
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
                throw invalid_argument("�����४�� ����"s);
            }
        }
        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        if (text[0] == '-' || text.empty()) {
            throw invalid_argument("�����४�� ����"s);
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

template <typename It> // 1.19.9 �뢮� ��࠭�栬� ��࠭�栬�
class Paginator {

public:

    Paginator() = default;

    Paginator(It range_begin, It range_end, size_t page_size) : page_size_(page_size) {
        size_t a = 0;
		vector<Document> v;
		for (auto it = range_begin; it != range_end; ++it) {
			v.push_back(*it);
			++a;
			if (a == page_size_ || it == range_end - 1) {
				documents_.push_back(v);
				v.clear();
				a = 0;
			}
		}
    } 

    auto begin() const {
        return documents_.begin();
    }

    auto end() const {
        return documents_.end();
    }

private:
    vector<vector<Document>> documents_;
    size_t page_size_;
}; 

ostream& operator<<(ostream& output, Document document) { // 1.19.9 �뢮� ��࠭�栬�
	output << "{ "s
	<< "document_id = "s << document.id << ", "s
	<< "relevance = "s << document.relevance << ", "s
	<< "rating = "s << document.rating << " }"s;
	return output;
}
	
template <typename Element> // 1.19.9 �뢮� ��࠭�栬�
ostream& operator<<(ostream& output, vector<Element> v) {
	for (auto document : v) {
		output << document;
	}
return output;
}

template <typename Container> // 1.19.9 �뢮� ��࠭�栬�
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server) : search_server_(search_server){}

    template <typename DocumentPredicate>
    vector<Document> AddFindRequest(const string& raw_query, DocumentPredicate document_predicate) {
        vector<Document> v = search_server_.FindTopDocuments(raw_query, document_predicate);
		QueryResult query;
		query.size = v.size();
		if (requests_.size() == min_in_day_) {
			requests_.pop_front();
		}
		requests_.push_back(query);
        return v;
    }

    vector<Document> AddFindRequest(const string& raw_query, DocumentStatus status) {
        return AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
    }

    vector<Document> AddFindRequest(const string& raw_query) {
        return AddFindRequest(raw_query, DocumentStatus::ACTUAL);
    }

    int GetNoResultRequests() const {
        int n = 0;
		for (auto a : requests_) {
			if (a.size == 0) {
				++n;
			}
		}
		return n;
    }
private:
    struct QueryResult {
        size_t size;
    };
    deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
    const SearchServer& search_server_;
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
    SearchServer server("� � ��"s);
    server.AddDocument(0, "���� ��� � ����� �襩���"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "������ ��� ������ 墮��"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    {
    vector<Document> v = server.FindTopDocuments("������"s);
    ASSERT_EQUAL(v.size(), 1);
    ASSERT_EQUAL(v[0].id, 1);
    ASSERT_EQUAL(v[0].rating, 5);
    }
    {
    vector<Document> v = server.FindTopDocuments("���"s);
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
    vector<Document> v = server.FindTopDocuments("��� -����"s);
    ASSERT_EQUAL(v.size(), 1);
    ASSERT_EQUAL(v[0].id, 1);
    ASSERT_EQUAL(v[0].rating, 5);
}

void TestSortByRelevance() {
    SearchServer server("� � ��"s);
    server.AddDocument(0, "���� ��� � ����� �襩���"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "������ ��� ������ 墮��"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    vector<Document> v = server.FindTopDocuments("������ ���"s);
    ASSERT_EQUAL(v.size(), 2);
    ASSERT(v[1].relevance <= v[0].relevance);
    ASSERT_EQUAL(v[1].id, 0);
}

void TestComputeRating() {
    SearchServer server("� � ��"s);
    server.AddDocument(0, "���� ��� � ����� �襩���"s,        DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "������ ��� ������ 墮��"s,       DocumentStatus::ACTUAL, {7, 2, 6});
    vector<Document> v = server.FindTopDocuments("���"s);
    ASSERT_EQUAL(v[0].rating, 5);
    ASSERT_EQUAL(v[1].rating, 3);
}

void TestFilteringByPredicate() {
    SearchServer server("� � ��"s);
    server.AddDocument(0, "���� ��� � ����� �襩���"s,       DocumentStatus::ACTUAL, {8, -2});
    server.AddDocument(1, "������ ��� ������ 墮��"s,      DocumentStatus::ACTUAL, {7, 2, 6});
    vector<Document> v = server.FindTopDocuments("���"s, [](int, DocumentStatus, int rating) {
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
    vector<Document> a = server.FindTopDocuments("���"s, DocumentStatus::ACTUAL);
    ASSERT_EQUAL(a.size(), 2);
    ASSERT_EQUAL(a[0].id, 1);
    vector<Document> b = server.FindTopDocuments("���"s, DocumentStatus::BANNED);
    ASSERT_EQUAL(b.size(), 2);
    ASSERT_EQUAL(b[0].id, 4);
    vector<Document> i = server.FindTopDocuments("᪢���"s, DocumentStatus::IRRELEVANT);
    ASSERT_EQUAL(i.size(), 1);
    ASSERT_EQUAL(i[0].id, 5);
    vector<Document> r = server.FindTopDocuments("���"s, DocumentStatus::REMOVED);
    ASSERT_EQUAL(r.size(), 1);
    ASSERT_EQUAL(r[0].id, 6);
}

void TestCalculatingRelevance() {
    SearchServer server("� � ��"s);
    server.AddDocument(0, "���� ��� � ����� �襩���"s,       DocumentStatus::ACTUAL, {8, -3});
    server.AddDocument(1, "������ ��� ������ 墮��"s,      DocumentStatus::ACTUAL, {7, 2, 7});
    server.AddDocument(2, "�宦���� ��� ��ࠧ�⥫�� �����"s,DocumentStatus::ACTUAL, {5, -12, 2, 1});
    server.AddDocument(3, "�宦���� ᪢��� �������"s,        DocumentStatus::BANNED, {9});
    vector<Document> v = server.FindTopDocuments("������ �宦���� ���"s);
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

// ==================== ��� �ਬ�� =========================

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
        cout << "�訡�� ���������� ���㬥�� "s << document_id << ": "s << e.what() << endl;
    }
}

void FindTopDocuments(const SearchServer& search_server, const string& raw_query) {
    cout << "�������� ���᪠ �� ������: "s << raw_query << endl;
    try {
        for (const Document& document : search_server.FindTopDocuments(raw_query)) {
            PrintDocument(document);
        }
    } catch (const exception& e) {
        cout << "�訡�� ���᪠: "s << e.what() << endl;
    }
}

void MatchDocuments(const SearchServer& search_server, const string& query) {
    try {
        cout << "���稭� ���㬥�⮢ �� ������: "s << query << endl;
        const int document_count = search_server.GetDocumentCount();
        for (int index = 0; index < document_count; ++index) {
            const int document_id = search_server.GetDocumentId(index);
            const auto [words, status] = search_server.MatchDocument(query, document_id);
            PrintMatchDocumentResult(document_id, words, status);
        }
    } catch (const exception& e) {
        cout << "�訡�� ���稭�� ���㬥�⮢ �� ����� "s << query << ": "s << e.what() << endl;
    }
}

/* int main() { // ��� �஢�ન �᭮���� ��⮤��
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
} */


/* int main() { // 1.19.9 �஢�ઠ �뢮�� ��࠭�栬�
    SearchServer search_server("and with"s);

    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});

    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);

    // �뢮��� �������� ���㬥��� �� ��࠭�栬
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        cout << *page << endl;
        cout << "Page break"s << endl;
    }
}  */

int main() { // �஢�ઠ ��।� ����ᮢ
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
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    return 0;
} 