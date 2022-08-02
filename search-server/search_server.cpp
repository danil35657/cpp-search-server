#include "search_server.h"

SearchServer::SearchServer(const std::string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {
}

SearchServer::SearchServer(const std::string_view stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {
}
        
void SearchServer::AddDocument (int document_id, const std::string_view document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("ID не может быть отрицательным"s);
    }
    if (documents_.count(document_id)) {
        throw std::invalid_argument("документ с таким ID уже есть"s);
    }
    if (!IsValidWord(document)) {
        throw std::invalid_argument("Некорректный ввод"s);
    }
    const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string_view& word : words) {
        auto it = all_words_.emplace(std::string(word));
        std::string_view word_view{*it.first};
        word_to_document_freqs_[word_view][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word_view] += inv_word_count;
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    document_ids_.emplace(document_id);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

template <class ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query, DocumentStatus status) const {
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>) {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
    }
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::parallel_policy>) {
        return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
    }
}

template <class ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, const std::string_view raw_query) const {
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::sequenced_policy>) {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }
    if constexpr (std::is_same_v<std::decay_t<ExecutionPolicy>, std::execution::parallel_policy>) {
        return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
    }
}

void SearchServer::RemoveDocument(int document_id) {
    if (documents_.count(document_id)) {
        for (const auto& [word, frequency] : document_to_word_freqs_.at(document_id)) {
            if (word_to_document_freqs_.at(word).size() > 1) {
                word_to_document_freqs_.at(word).erase(document_id);
            } else {
                word_to_document_freqs_.erase(word);
                all_words_.erase(std::string(word));
            }
        }
        document_to_word_freqs_.erase(document_id);
        documents_.erase(document_id);
        document_ids_.erase(document_id);
    }
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id) {
    return RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id) {
    if (documents_.count(document_id)) {
        std::vector<std::string_view> document_words(document_to_word_freqs_.at(document_id).size());
        std::transform(document_to_word_freqs_.at(document_id).begin(), document_to_word_freqs_.at(document_id).end(), document_words.begin(), [](auto& word) { return word.first; } );
        std::for_each(policy, document_words.begin(), document_words.end(), [this, document_id] (auto data) {
            if (word_to_document_freqs_.at(data).size() > 1) {
                word_to_document_freqs_.at(data).erase(document_id);
            } else {
                word_to_document_freqs_.erase(data);
                all_words_.erase(std::string(data));
            }
        } );
        document_to_word_freqs_.erase(document_id);
        documents_.erase(document_id);
        document_ids_.erase(document_id);
    }
}

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string_view, double> frequencis;
    if (document_to_word_freqs_.count(document_id)) {
        return document_to_word_freqs_.at(document_id);
    }
    return frequencis;
}

using matching_result = std::tuple<std::vector<std::string_view>, DocumentStatus>;

matching_result SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const {
    
    if (!documents_.count(document_id)) {
        throw std::out_of_range("Нет такого документа"s);
    }
    
    const Query query = ParseQuery(raw_query, true);
    
    const auto status = documents_.at(document_id).status;
    
    std::vector<std::string_view> matched_words;
    
    for (const std::string_view word : query.minus_words) {
        if (document_to_word_freqs_.at(document_id).count(word)) {
            return {matched_words, status};
        }
    }
    
    matched_words.reserve(query.plus_words.size());
    for (const std::string_view word : query.plus_words) {
        if (document_to_word_freqs_.at(document_id).count(word)) {
            matched_words.push_back(word);
        }
    }
    return {matched_words, status};
}

matching_result SearchServer::MatchDocument(std::execution::sequenced_policy policy, const std::string_view raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}

matching_result SearchServer::MatchDocument(std::execution::parallel_policy policy, const std::string_view raw_query, int document_id) const {
    
    if (!documents_.count(document_id)) {
        throw std::out_of_range("Нет такого документа"s);
    }
    
    const Query query = ParseQuery(raw_query, false);
    
    const auto status = documents_.at(document_id).status;
    
    const auto word_check = [this, document_id] (const std::string_view word) {return document_to_word_freqs_.at(document_id).count(word);};
    
    std::vector<std::string_view> words;
    
    for (const std::string_view word : query.minus_words) {
        if (document_to_word_freqs_.at(document_id).count(word)) {
            return {words, status};
        }
    } // здесь это работает быстрее чем алгоритмы типа any_of с execution::par
    
    std::vector<std::string_view> matched_words(query.plus_words.size());
    auto words_end = std::copy_if(policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), word_check);
    std::sort(policy, matched_words.begin(), words_end);
    matched_words.erase(std::unique(matched_words.begin(), words_end), matched_words.end());
    return {matched_words, status};
}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}
    
std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

bool SearchServer::IsStopWord(const std::string_view word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const {
    std::vector<std::string_view> words;
    for (const std::string_view word : SplitIntoWords(text)) {
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
    bool is_minus = false;
    if (!IsValidWord(text)) {
        throw std::invalid_argument("Некорректный ввод: "s + std::string(text));
    }
    if (text[0] == '-') {
        is_minus = true;
        text.remove_prefix(1);
    }
    if (text[0] == '-' || text.empty()) {
        throw std::invalid_argument("Некорректный ввод"s);
    }
    return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text, bool sorted) const {
    Query query;
    for (const std::string_view word : SplitIntoWords(text)) {
        const QueryWord query_word = ParseQueryWord(word);
        if (!query_word.is_stop) {
            query_word.is_minus ? query.minus_words.push_back(query_word.data) : query.plus_words.push_back(query_word.data);
        }
    }
    
    if (sorted) {
        std::sort(query.plus_words.begin(), query.plus_words.end());
        query.plus_words.erase(std::unique(query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());
        std::sort(query.minus_words.begin(), query.minus_words.end());
        query.minus_words.erase(std::unique(query.minus_words.begin(), query.minus_words.end()), query.minus_words.end());
    }
    
    return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}