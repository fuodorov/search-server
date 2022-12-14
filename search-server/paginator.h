#pragma once

#ifndef CPP_SEARCH_SERVER_PAGINATOR_H
#define CPP_SEARCH_SERVER_PAGINATOR_H

#include <cmath>
#include <fstream>
#include <iterator>
#include <utility>
#include <vector>

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator pageBeginIt, Iterator pageEndIt) : _iteratorRange(pageBeginIt, pageEndIt) {}

    Iterator begin() const {
        return _iteratorRange.first;
    }

    Iterator end() const {
        return _iteratorRange.second;
    }

    size_t size() const {
        return std::abs(std::distance(_iteratorRange.first, _iteratorRange.second));
    }

private:
    std::pair<Iterator, Iterator> _iteratorRange;
};

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        for (size_t left = distance(begin, end); left > 0;) {
            const size_t current_page_size = std::min(page_size, left);
            const Iterator current_page_end = next(begin, current_page_size);
            pages_.push_back({begin, current_page_end});

            left -= current_page_size;
            begin = current_page_end;
        }
    }

    auto begin() const {
        return pages_.begin();
    }

    auto end() const {
        return pages_.end();
    }

    size_t size() const {
        return pages_.size();
    }

private:
    std::vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}


template <typename Iterator>
std::ostream& operator<<(std::ostream& os, const IteratorRange<Iterator>& range) {
    for (auto it = range.begin(); it != range.end(); ++it) {
        os << *it;
    }
    return os;
}

#endif //CPP_SEARCH_SERVER_PAGINATOR_H
