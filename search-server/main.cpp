#include <algorithm>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;


const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPS = 1e-6;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
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

struct Document {
    int id;
    double relevance;
    int rating;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED,
};

class SearchServer {
public:
    void SetStopWords(const string& text) {
        for (const string& word : SplitIntoWords(text)) {
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words) {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }
        documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
            return abs(lhs.relevance - rhs.relevance) < EPS ? lhs.rating > rhs.rating : lhs.relevance > rhs.relevance;
        });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
            return document_status == status;
        });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    int GetDocumentCount() const {
        return documents_.size();
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
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

private:
    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    set<string> stop_words_;
    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        return ratings.empty() ? 0 : accumulate(ratings.begin(), ratings.end(), 0) * 1.0 / ratings.size();
    }

    struct QueryWord {
        string data;
        bool is_minus;
        bool is_stop;
    };

    QueryWord ParseQueryWord(string text) const {
        bool is_minus = false;

        if (text[0] == '-') {
            is_minus = true;
            text = text.substr(1);
        }
        return {text, is_minus, IsStopWord(text)};
    }

    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    Query ParseQuery(const string& text) const {
        Query query;
        for (const string& word : SplitIntoWords(text)) {
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

    double ComputeWordInverseDocumentFreq(const string& word) const {
        return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate document_predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
            for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word)) {
                const auto& document_data = documents_.at(document_id);
                if (document_predicate(document_id, document_data.status, document_data.rating)) {
                    document_to_relevance[document_id] += term_freq * inverse_document_freq;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(document_id);
            }
        }

        vector<Document> matched_documents;
        for (const auto [document_id, relevance] : document_to_relevance) {
            matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
        }
        return matched_documents;
    }
};

template <typename T, typename U>
ostream& operator<<(ostream& out, const pair<T, U>& container) {
    return out << container.first << ": " << container.second;
}

template <typename T>
void Print(ostream& out, const T& container) {
    bool is_first = true;
    for (const auto& element : container) {
        if (is_first) {
            out << element;
            is_first = false;
        }
        else {
            out << ", "s << element;
        }
    }
}

template <typename T>
ostream& operator<<(ostream& out, const vector<T>& container) {
    out << "["s;
    Print(out, container);
    out << "]"s;
    return out;
}

template <typename T>
ostream& operator<<(ostream& out, const set<T>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

template <typename T, typename U>
ostream& operator<<(ostream& out, const map<T, U>& container) {
    out << "{"s;
    Print(out, container);
    out << "}"s;
    return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cerr << boolalpha;
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cerr << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func,
                unsigned line, const string& hint) {
    if (!value) {
        cerr << file << "("s << line << "): "s << func << ": "s;
        cerr << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cerr << " Hint: "s << hint;
        }
        cerr << endl;
        abort();
    }
}

template <typename T>
void RunTestImpl(T func, const string& func_str) {
    cerr << func_str << " RUN"s << endl;
    func();
    cerr << func_str << " OK" << endl;
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))
#define RUN_TEST(func) RunTestImpl((func), #func)

struct TEST_DOCUMENT {int id; string content; DocumentStatus status; vector<int> ratings;};
const vector<TEST_DOCUMENT> TEST_DOCUMENTS = {
        {1,  "funny pet and nasty rat"s,   DocumentStatus::ACTUAL,  {7,  2, 7}},
        {2,  "funny pet with curly hair"s, DocumentStatus::ACTUAL,  {10, 5}},
        {3,  "nasty rat with curly hair"s, DocumentStatus::ACTUAL,  {6}},
        {4,  "funny pet and curly hair"s,  DocumentStatus::ACTUAL,  {4,  6}},
        {5,  "funny pet with curly hair"s, DocumentStatus::ACTUAL,  {4}},
        {6,  "curly hair"s,                DocumentStatus::ACTUAL,  {1}},
        {7,  "funny pet"s,                 DocumentStatus::ACTUAL,  {10}},
        {8,  "nasty rat"s,                 DocumentStatus::ACTUAL,  {}},
        {9,  "funny pet and nasty rat"s,   DocumentStatus::BANNED,  {7,  2, 7}},
        {10, "funny pet with curly hair"s, DocumentStatus::BANNED,  {10, 5}},
        {11, "nasty rat with curly hair"s, DocumentStatus::BANNED,  {6}},
        {12, "funny pet and curly hair"s,  DocumentStatus::BANNED,  {4,  6}},
        {13, "funny pet with curly hair"s, DocumentStatus::BANNED,  {4}},
        {14, "curly hair"s,                DocumentStatus::BANNED,  {1}},
        {15, "funny pet"s,                 DocumentStatus::BANNED,  {10}},
        {16, "nasty rat"s,                 DocumentStatus::BANNED,  {}},
        {17, "funny pet and nasty rat"s,   DocumentStatus::REMOVED, {7,  2, 7}},
        {18, "funny pet with curly hair"s, DocumentStatus::REMOVED, {10, 5}},
        {19, "nasty rat with curly hair"s, DocumentStatus::REMOVED, {6}},
        {20, "funny pet and curly hair"s,  DocumentStatus::REMOVED, {4,  6}},
        {21, "funny pet with curly hair"s, DocumentStatus::REMOVED, {4}},
        {22, "curly hair"s,                DocumentStatus::REMOVED, {1}},
        {23, "funny pet"s,                 DocumentStatus::REMOVED, {10}},
        {24, "nasty rat"s,                 DocumentStatus::REMOVED, {}},
        {25, "funny pet and nasty rat"s,   DocumentStatus::IRRELEVANT, {7,  2, 7}},
        {26, "funny pet with curly hair"s, DocumentStatus::IRRELEVANT, {10, 5}},
        {27, "nasty rat with curly hair"s, DocumentStatus::IRRELEVANT, {6}},
        {28, "funny pet and curly hair"s,  DocumentStatus::IRRELEVANT, {4,  6}},
        {29, "funny pet with curly hair"s, DocumentStatus::IRRELEVANT, {4}},
        {30, "curly hair"s,                DocumentStatus::IRRELEVANT, {1}},
        {31, "funny pet"s,                 DocumentStatus::IRRELEVANT, {10}},
        {32, "nasty rat"s,                 DocumentStatus::IRRELEVANT, {}},
};

struct TEST_COUNT_STOP_WORD_REQUEST {string word; int count;};
const vector<TEST_COUNT_STOP_WORD_REQUEST> TEST_COUNT_STOP_WORD_REQUESTS = {
        {"and"s,    2},
        {"with"s,   3},
        {"unknown"s, 0},
};

struct TEST_MATCH_DOCUMENT_REQUEST {string query; vector<int> document_ids;};
const vector<TEST_MATCH_DOCUMENT_REQUEST> TEST_MATCH_DOCUMENT_REQUESTS = {
        {"funny pet"s, {1, 2, 4, 5, 7}},
        {"curly hair"s, {2, 3, 4, 5, 6}},
        {"nasty rat"s, {1, 3, 8}},
        {"unknown"s, {}},
};
const vector<TEST_MATCH_DOCUMENT_REQUEST> TEST_MATCH_DOCUMENT_REQUESTS_WITH_MINUS_WORDS = {
        {"funny pet -nasty"s, {2, 4, 5, 7}},
        {"curly hair -nasty"s, {2, 4, 5, 6}},
        {"nasty rat -funny"s, {3, 8}},
        {"unknown"s, {}},
};

SearchServer CreateTestServer() {
    SearchServer server;
    for (const auto& doc : TEST_DOCUMENTS) {
        server.AddDocument(doc.id, doc.content, doc.status, doc.ratings);
    }
    return server;
}

void TestDocumentSearchByQuery() {
    SearchServer server = CreateTestServer();

    {
        for (const TEST_MATCH_DOCUMENT_REQUEST &request: TEST_MATCH_DOCUMENT_REQUESTS) {
            vector<int> document_ids;
            for (const auto& doc : server.FindTopDocuments(request.query)) {
                document_ids.push_back(doc.id);
            }
            sort(document_ids.begin(), document_ids.end());
            ASSERT_EQUAL_HINT(document_ids, request.document_ids,
                              "Incorrect document ids for query '"s + request.query + "'"s);
        }
    }
}

void TestDocumentSearchByStatus() {
    SearchServer server = CreateTestServer();

    {
        for (const TEST_MATCH_DOCUMENT_REQUEST &request: TEST_MATCH_DOCUMENT_REQUESTS) {
            for (const auto &status: {DocumentStatus::ACTUAL, DocumentStatus::IRRELEVANT, DocumentStatus::BANNED}) {
                for (const auto &doc: server.FindTopDocuments(request.query, status)) {
                    ASSERT_EQUAL_HINT(static_cast<int>(TEST_DOCUMENTS[doc.id - 1].status), static_cast<int>(status),
                                      "Incorrect document status for query '"s + request.query + "'"s);
                }
            }
        }
    }
}

void TestDocumentSearchByPredicate() {
    SearchServer server = CreateTestServer();

    {
        for (const TEST_MATCH_DOCUMENT_REQUEST &request: TEST_MATCH_DOCUMENT_REQUESTS) {
            for (const auto &doc: server.FindTopDocuments(request.query,
                                                          [](int document_id, DocumentStatus status, int rating) {
                                                              return document_id % 2 == 0;
                                                          })) {
                ASSERT_EQUAL_HINT(doc.id % 2, 0, "Incorrect document id for query '"s + request.query + "'"s);
            }
        }
    }
}

void TestCalculateDocumentRating() {
    SearchServer server = CreateTestServer();

    {
        for (const TEST_MATCH_DOCUMENT_REQUEST &request: TEST_MATCH_DOCUMENT_REQUESTS) {
            vector<pair<int, int>> document_ids_and_ratings;
            for (const auto& doc : server.FindTopDocuments(request.query)) {
                document_ids_and_ratings.push_back({doc.id, doc.rating});
            }
            for (const auto& [id, rating] : document_ids_and_ratings) {
                if (rating != 0) {
                    ASSERT_EQUAL_HINT(rating,
                                      accumulate(TEST_DOCUMENTS[id - 1].ratings.begin(),
                                                 TEST_DOCUMENTS[id - 1].ratings.end(), 0) /
                                      TEST_DOCUMENTS[id - 1].ratings.size(),
                                      "Incorrect rating for document id = "s + to_string(id));
                } else {
                    ASSERT_EQUAL_HINT(rating, 0,
                                      "Incorrect rating for document id = "s + to_string(id));
                }
            }
        }
    }
}

void TestExcludeStopWordsFromAddedDocumentContent() {
    SearchServer server = CreateTestServer();

    {
        for (const TEST_COUNT_STOP_WORD_REQUEST &request: TEST_COUNT_STOP_WORD_REQUESTS) {
            ASSERT_EQUAL_HINT(server.FindTopDocuments(request.word).size(), request.count,
                              "Incorrect count of documents with word '"s + request.word + "'"s);
        }
    }

    {
        for (const TEST_COUNT_STOP_WORD_REQUEST &request: TEST_COUNT_STOP_WORD_REQUESTS) {
            server.SetStopWords({request.word});
            ASSERT_EQUAL_HINT(server.FindTopDocuments(request.word).size(), 0,
                              "Incorrect count of documents with word '"s + request.word + "' after excluding it"s);
        }
    }
}

void TestExcludeDocumentsWithMinusWordsFromAddedDocumentContent() {
    SearchServer server = CreateTestServer();

    {
        for (const TEST_MATCH_DOCUMENT_REQUEST &request: TEST_MATCH_DOCUMENT_REQUESTS_WITH_MINUS_WORDS) {
            vector<int> document_ids;
            for (const auto& doc : server.FindTopDocuments(request.query)) {
                document_ids.push_back(doc.id);
            }
            sort(document_ids.begin(), document_ids.end());
            ASSERT_EQUAL_HINT(document_ids, request.document_ids,
                              "Incorrect document ids for query '"s + request.query + "'"s);
        }
    }
}

void TestSearchServer() {
    RUN_TEST(TestDocumentSearchByQuery);
    RUN_TEST(TestDocumentSearchByStatus);
    RUN_TEST(TestDocumentSearchByPredicate);
    RUN_TEST(TestCalculateDocumentRating);
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    RUN_TEST(TestExcludeDocumentsWithMinusWordsFromAddedDocumentContent);
}

int main() {
    TestSearchServer();
    cout << "Search server testing finished"s << endl;
}