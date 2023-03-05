#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
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

std::vector<std::string_view> SplitIntoWordsView(std::string_view str) {
    std::vector<std::string_view> words;
    while (!str.empty()) {
        const size_t space_pos = str.find(' ');
        words.push_back(str.substr(0, space_pos));
        str.remove_prefix(space_pos != str.npos ? space_pos + 1 : str.size());
    }
    return words;
}