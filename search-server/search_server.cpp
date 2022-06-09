#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {
}
        
void SearchServer::AddDocument (int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("ID �� ����� ���� ����⥫��"s);
    } else if (documents_.count(document_id)) {
        throw std::invalid_argument("���㬥�� � ⠪�� ID 㦥 ����"s);
    } else if (!IsValidWord(document)) {
        throw std::invalid_argument("�����४�� ����"s);
    }
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        word_freqs_[document_id][word] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

void SearchServer::RemoveDocument(int document_id) {
    if (documents_.count(document_id)) {
        for (const auto& [word, frequency] : word_freqs_.at(document_id)) {
            word_to_document_freqs_.erase(word);
        }
        word_freqs_.erase(document_id);
        documents_.erase(document_id);
        document_ids_.erase(document_id);
    }
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string, double> frequencis;
    if (word_freqs_.count(document_id)) {
        return word_freqs_.at(document_id);
    }
    return frequencis;
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    const Query query = ParseQuery(raw_query);
    std::vector<std::string> matched_words;
    for (const std::string& word : query.plus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }
    for (const std::string& word : query.minus_words) {
        if (word_to_document_freqs_.count(word) == 0) {
            continue;
        }
        if (word_to_document_freqs_.at(word).count(document_id)) {
            matched_words.clear();
            break;
        }
    }
    return std::tuple(matched_words, documents_.at(document_id).status);
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}
    
std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty()) {
        return 0;
    }
    return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    bool is_minus = false;
    for (char c : text) {
        if (c >= '\0' && c < ' ') {
            throw std::invalid_argument("�����४�� ����"s);
        }
    }
    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }
    if (text[0] == '-' || text.empty()) {
        throw std::invalid_argument("�����४�� ����"s);
    }
    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query;
    for (const std::string& word : SplitIntoWords(text)) {
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

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
