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
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate){
        auto result = search_server_.FindTopDocuments(raw_query, document_predicate);
        if (requests_.size() >= min_in_day_) {
            if (requests_[0].result.size() != 0) {
                --empty_count_;
                requests_.pop_back();
            }
        }
        if (result.empty()) {
            ++empty_count_;
        }
        requests_.push_front({result});
        return result;
    }

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    struct QueryResult {
        std::vector<Document> result;
    };

    std::deque<QueryResult> requests_;
    const static int min_in_day_ = MIN_IN_DAY;
    int empty_count_ = 0;
    const SearchServer& search_server_;
};
