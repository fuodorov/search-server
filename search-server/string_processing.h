#pragma once

#ifndef CPP_SEARCH_SERVER_STRING_PROCESSING_H
#define CPP_SEARCH_SERVER_STRING_PROCESSING_H

#include <iostream>
#include <set>
#include <string>
#include <vector>

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
    std::set<std::string> non_empty_strings;

    for (const std::string& str : strings) {
        if (!str.empty()) {
            non_empty_strings.insert(str);
        }
    }
    return non_empty_strings;
}

std::vector<std::string> SplitIntoWords(const std::string& text);


#endif //CPP_SEARCH_SERVER_STRING_PROCESSING_H
