#include "remove_duplicates.h"


void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> s;
    for (auto it = search_server.begin(); it != search_server.end(); ++it) {
        auto a = search_server.GetWordFrequencies(*it);
        for (auto iter = next(it); iter != search_server.end(); ++iter) {
            auto b = search_server.GetWordFrequencies(*iter);
            if (a.size() != b.size()) {
                continue;
            }
            auto c = a.begin();
            auto d = b.begin();
            for (int n = 0; n < a.size(); ++n) {
                if (c->first != d->first) {
                    break;
                }
                c = next(c);
                d = next(d);
            }
            if (c == a.end()) {
                s.emplace(*iter);
            }
        }
    }
    for (const int a : s) {
        std::cout << "Found duplicate document id "s << a << std::endl;
        search_server.RemoveDocument(a);
    }
}