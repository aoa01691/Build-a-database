#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

// Simple key-value pair struct (our "index" entry)
struct Entry {
    std::string key;
    std::string value;
};

// Linear index (no maps/dictionaries). Last write wins by overwriting existing key.
class KVIndex {
private:
    std::vector<Entry> entries;

public:
    // Find index of key, or -1 if not found
    int findKey(const std::string& key) const {
        for (int i = 0; i < (int)entries.size(); i++) {
            if (entries[i].key == key) return i;
        }
        return -1;
    }

    void set(const std::string& key, const std::string& value) {
        int idx = findKey(key);
        if (idx == -1) {
            entries.push_back({key, value});
        } else {
            entries[idx].value = value; // last write wins
        }
    }

    bool get(const std::string& key, std::string& outValue) const {
        int idx = findKey(key);
        if (idx == -1) return false;
        outValue = entries[idx].value;
        return true;
    }
};
