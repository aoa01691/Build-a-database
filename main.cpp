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

// Append-only storage file
static const char* DB_FILE = "data.db";

// Parse a log line of the form: SET <key> <value...>
// We store lines exactly like: SET key value\n
// Value may contain spaces, so we parse key as the 2nd token and the rest as value.
bool parseSetLine(const std::string& line, std::string& keyOut, std::string& valueOut) {
    // Expect starts with "SET "
    if (line.rfind("SET ", 0) != 0) return false;

    // Remove trailing newline if present
    std::string s = line;
    if (!s.empty() && (s.back() == '\n' || s.back() == '\r')) {
        while (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    }

    // Split into "SET", key, and value remainder
    std::istringstream iss(s);
    std::string cmd;
    if (!(iss >> cmd)) return false;
    if (cmd != "SET") return false;

    if (!(iss >> keyOut)) return false;

    // Remainder (including spaces) is the value
    std::string rest;
    std::getline(iss, rest); // begins with a leading space if value exists
    if (!rest.empty() && rest[0] == ' ') rest.erase(0, 1);
    valueOut = rest; // may be empty string
    return true;
}

// Replay log on startup to rebuild index
void replayLog(KVIndex& index) {
    std::ifstream in(DB_FILE);
    if (!in.is_open()) return; // no file yet is fine

    std::string line;
    while (std::getline(in, line)) {
        // getline removes '\n'
        std::string key, value;
        // rebuild line with no newline already; parseSetLine expects "SET ..."
        if (line.rfind("SET ", 0) == 0) {
            // parseSetLine expects potentially newline; safe either way
            if (parseSetLine(line, key, value)) {
                index.set(key, value);
            }
        }
        // Ignore unknown lines for robustness
    }
}

bool appendSetToDisk(const std::string& key, const std::string& value) {
    // Append-only, persisted immediately
    std::ofstream out(DB_FILE, std::ios::app);
    if (!out.is_open()) return false;

    out << "SET " << key;
    if (!value.empty()) out << " " << value;
    out << "\n";

    out.flush();
    return out.good();
}

// Parse user command line for interactive / piped input
// Commands:
// SET <key> <value...>
// GET <key>
// EXIT
int main() {
    KVIndex index;
    replayLog(index);

    std::string line;
    while (std::getline(std::cin, line)) {
        // Trim CR if running on Windows-style inputs
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        // Determine command
        if (line == "EXIT") {
            break;
        }

        if (line.rfind("SET ", 0) == 0) {
            std::string key, value;
            if (!parseSetLine(line, key, value)) {
                // For black-box testing, keep output minimal/consistent
                std::cout << "ERROR\n";
                continue;
            }

            // Persist first (so it survives crash between disk and memory)
            if (!appendSetToDisk(key, value)) {
                std::cout << "ERROR\n";
                continue;
            }

            // Update in-memory index
            index.set(key, value);
            std::cout << "OK\n";
            continue;
        }

        if (line.rfind("GET ", 0) == 0) {
            std::istringstream iss(line);
            std::string cmd, key;
            iss >> cmd >> key;
            if (cmd != "GET" || key.empty()) {
                std::cout << "ERROR\n";
                continue;
            }

            std::string value;
            if (index.get(key, value)) {
                std::cout << value << "\n";
            } else {
                std::cout << "NULL\n";
            }
            continue;
        }

        // Unknown command
        std::cout << "ERROR\n";
    }

    return 0;
}
