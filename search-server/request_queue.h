#pragma once

#include <algorithm>
#include <deque>
#include <string>
#include <vector>

#include "search_server.h"
#include "document.h"
#include "config.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server) : search_server_(search_server) {}

    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    std::deque<bool> requests_;
    const static int min_in_day_ = MIN_IN_DAY;
    const SearchServer& search_server_;
    int minutes = 0;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    std::vector<Document> docs = search_server_.FindTopDocuments(raw_query, document_predicate);

    minutes++;

    if (minutes > min_in_day_) {
        requests_.pop_front();
        minutes--;
    }

    requests_.push_back(docs.size() > 0);

    return docs;
}
