#include "search_server.h"

int SearchServer::GetDocumentCount() const {
    return documents_.size();
}

void SearchServer::AddDocument(int document_id, const std::string& document,
                               DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Document id "s + std::to_string(document_id) + " is less than zero"s);
    }
    if (documents_.count(document_id) > 0) {
        throw std::invalid_argument("Document id "s + std::to_string(document_id) + " is already exists"s);
    }
    const std::vector<std::string> words = SplitIntoWordsNoStop(document);
    document_ids_.insert(document_id);
    const double inv_word_count = 1.0 / words.size();
    for (const std::string& word : words) {
        word_to_document_freqs_[word][document_id] += inv_word_count;
        document_to_word_freqs_[document_id][word] += word_to_document_freqs_[word][document_id];
    }
    documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus status) const {
    return SearchServer::FindTopDocuments(raw_query,
                                          [status](int document_id, DocumentStatus document_status, int rating) {
        return document_status == status;
    });
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return SearchServer::FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    return MatchDocument(std::execution::seq, raw_query, document_id);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::execution::sequenced_policy&, const std::string& raw_query, int document_id) const {
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
    return {matched_words, documents_.at(document_id).status};
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::execution::parallel_policy&, const std::string& raw_query, int document_id) const {
    const Query query = ParseQueryParallel(raw_query);
    std::vector<std::string> matched_words;

    if (any_of(query.minus_words.begin(), query.minus_words.end(), [this, document_id](const std::string& word) {
        return word_to_document_freqs_.count(word) > 0 && word_to_document_freqs_.at(word).count(document_id) > 0;
    })) {
        return {matched_words, documents_.at(document_id).status};
    }

    matched_words.reserve(query.plus_words.size());
    
    const auto& it = std::copy_if(query.plus_words.begin(), query.plus_words.end(), matched_words.begin(), [this, document_id](const std::string& word) {
        return word_to_document_freqs_.count(word) > 0 && word_to_document_freqs_.at(word).count(document_id) > 0;
    });
    
    matched_words.erase(it, matched_words.end());
    
    std::sort(std::execution::par, matched_words.begin(), matched_words.end());
    const auto& itr = std::unique(matched_words.begin(), matched_words.end());
    matched_words.erase(itr, matched_words.end());
    
    return { matched_words, documents_.at(document_id).status };

}

std::set<int>::const_iterator SearchServer::begin() const {
    return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const {
    return document_ids_.end();
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    if (document_to_word_freqs_.count(document_id)) {
        return document_to_word_freqs_.at(document_id);
    } else {
        static const std::map<std::string, double> empty;
        return empty;
    }
}

void SearchServer::RemoveDocument(int document_id) {
    return RemoveDocument(std::execution::seq, document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id) {
    if (document_to_word_freqs_.count(document_id) == 0) {
        throw std::invalid_argument("Document id "s + std::to_string(document_id) + " not found"s);
    }
    for (const auto& [word, _] : document_to_word_freqs_.at(document_id)) {
        word_to_document_freqs_.at(word).erase(document_id);
    }
    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id) {
    if (document_to_word_freqs_.count(document_id) == 0) {
        throw std::invalid_argument("Document id "s + std::to_string(document_id) + " not found"s);
    }
    std::map<std::string, double> word_freqs = document_to_word_freqs_.at(document_id);
    std::vector<const std::string*> words_to_erase(word_freqs.size());
    std::transform(std::execution::par, word_freqs.begin(), word_freqs.end(), words_to_erase.begin(),
                   [](const auto& word_freq) { return &word_freq.first; });
    std::for_each(std::execution::par, words_to_erase.begin(), words_to_erase.end(),
                  [document_id, this](const std::string* word) {
        word_to_document_freqs_.at(*word).erase(document_id);
    });
    document_to_word_freqs_.erase(document_id);
    documents_.erase(document_id);
    document_ids_.erase(document_id);
}

bool SearchServer::IsValidWord(const std::string& word) {
    return none_of(word.begin(), word.end(), [](char c) {
        return c > '\0' && c < ' ';
    });
}

bool SearchServer::IsStopWord(const std::string& word) const {
    return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (!IsValidWord(word)) {
            throw std::invalid_argument("Word "s + word + " includes special symbols"s);
        }
        if (!IsStopWord(word)) {
            words.push_back(word);
        }
    }
    return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    return ratings.empty() ? 0 : accumulate(ratings.begin(), ratings.end(), 0) * 1.0 / ratings.size();
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const {
    if (text.empty()) {
        throw std::invalid_argument("Query word is empty"s);
    }

    bool is_minus = false;

    if (text[0] == '-') {
        is_minus = true;
        text = text.substr(1);
    }

    if (text.empty()) {
        throw std::invalid_argument("Query word i`s empty"s);
    }
    if (!IsValidWord(text)) {
        throw std::invalid_argument("Query word "s + text + " includes special symbols"s);
    }
    if (text[0] == '-') {
        throw std::invalid_argument("Query word "s + text + " starts with minus"s);
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

SearchServer::Query SearchServer::ParseQueryParallel(const std::string& text) const {
    Query query;
    std::vector<std::string> words = SplitIntoWords(text);
    std::vector<QueryWord> query_words(words.size());
    std::transform(std::execution::par, words.begin(), words.end(), query_words.begin(),
                   [this](const std::string& word) { return ParseQueryWord(word); });
    for (const QueryWord& query_word : query_words) {
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

double SearchServer::ComputeInverseDocumentFreq(const std::string& word) const {
    return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
