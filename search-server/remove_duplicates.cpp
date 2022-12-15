#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::vector<int> document_ids_to_remove;
    std::set<std::set<std::string>> unique_words;

    for (const auto document_id : search_server) {
        std::set<std::string> words;

        for (const auto& [word, document_freqs] : search_server.GetWordFrequencies(document_id)) {
            words.insert(word);
        }

        if (unique_words.count(words) > 0) {
            document_ids_to_remove.push_back(document_id);
        } else {
            unique_words.insert(words);
        }
    }

    for (const auto document_id : document_ids_to_remove) {
        std::cout << "Found duplicate document id " << document_id << std::endl;
        search_server.RemoveDocument(document_id);
    }
}
