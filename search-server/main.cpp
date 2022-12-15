#include "request_queue.h"
#include "search_server.h"
#include "log_duration.h"

int main() {
    SearchServer search_server("and in at"s);

    search_server.AddDocument(6, "curly cat curly tail"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(8, "big cat fancy collar "s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(23, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, {1, 1, 1});

    {
        LogDuration guard("Long task", std::cout);
        search_server.FindTopDocuments("cat -dog"s);
    }

    {
        LOG_DURATION_STREAM("Long task", std::cout);
        search_server.FindTopDocuments("cat -dog"s);
    }
}
