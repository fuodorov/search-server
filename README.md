# Search Server
SearchServer is a keyword-based document search system.

## Main functions

* ranking of search results by statistical measure TF-IDF;
* stop word processing (not taken into account by search engine and do not affect search results);
* processing of minus-words (documents containing minus-words will not be included into search results);
* query queue creation and handling;
* removing duplicate documents;
* paginated separation of search results;
* ability to work in multi-threaded mode;

## Working Principle
Creating an instance of `SearchServer` class. A string with stop words separated by spaces is passed to the constructor. Instead of string you can pass any container (with sequential access to elements with possibility to use it in for-range loop)

The `AddDocument` method is used to add documents to be searched. The document's id, status, rating, and the document itself are passed to the method in string format.

The `FindTopDocuments` method returns a vector of documents, according to the matching keywords passed. The results are sorted by TF-IDF statistical measure. Additional filtering of documents by id, status and rating is possible. The method is implemented in both single-threaded and multi-threaded versions.

The `RequestQueueue` class implements a queue of requests to the search server with search results saved.
