#include "document.h"

using namespace std::literals;

std::ostream& operator<<(std::ostream& out, const Document& document) {
    return out << "{"
               << "document_id = " << document.id << ", "
               << "relevance = " << document.relevance << ", "
               << "rating = " << document.rating
               << "}";
}
