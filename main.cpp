#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

struct Entry {
    std::string key;
    std::string value;
};

class KVIndex {
private:
    std::vector<Entry> entries;

public:
    int findKey(const std::string& key) const {
        for (int i = 0; i < static_cast<int>(entries.size()); i++) {
            if (entries[i].key == key) {
                return i;
            }
        }
        return -1;
    }

    void set(const std::string& key, const std::string& value) {
        int idx = findKey(key);
        if (idx == -1) {
            entries.push_back({key, value});
        } else {
            entries[idx].value = value;
        }
    }

    bool get(const std::string& key, std::string& outValue) const {
        int idx = findKey(key);
        if (idx == -1) {
            return false;
        }
        outValue = entries[idx].value;
        return true;
    }
};

static const char* DB_FILE = "data.db";

bool parseSetLine(const std::string& line, std::string& keyOut, std::string& valueOut) {
    if (line.rfind("SET ", 0) != 0) {
        return false;
    }

    std::istringstream iss(line);
    std::string cmd;

    if (!(iss >> cmd) || cmd != "SET") {
        return false;
    }

    if (!(iss >> keyOut)) {
        return false;
    }

    std::getline(iss, valueOut);
    if (!valueOut.empty() && valueOut[0] == ' ') {
        valueOut.erase(0, 1);
    }

    return true;
}

void replayLog(KVIndex& index) {
    std::ifstream in(DB_FILE);
    if (!in.is_open()) {
        return;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string key, value;
        if (parseSetLine(line, key, value)) {
            index.set(key, value);
        }
    }
}

bool appendSetToDisk(const std::string& key, const std::string& value) {
    std::ofstream out(DB_FILE, std::ios::app);
    if (!out.is_open()) {
        return false;
    }

    out << "SET " << key;
    if (!value.empty()) {
        out << " " << value;
    }
    out << "\n";
    out.flush();

    return out.good();
}

int main() {
    KVIndex index;
    replayLog(index);

    std::string line;
    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        if (line == "EXIT") {
            break;
        }

        if (line.rfind("SET ", 0) == 0) {
            std::string key, value;

            if (!parseSetLine(line, key, value)) {
                std::cout << "ERROR" << std::endl;
                continue;
            }

            if (!appendSetToDisk(key, value)) {
                std::cout << "ERROR" << std::endl;
                continue;
            }

            index.set(key, value);
            std::cout << "OK" << std::endl;
            continue;
        }

        if (line.rfind("GET ", 0) == 0) {
            std::istringstream iss(line);
            std::string cmd, key;

            if (!(iss >> cmd >> key) || cmd != "GET") {
                std::cout << "ERROR" << std::endl;
                continue;
            }

            std::string value;
            if (index.get(key, value)) {
                std::cout << value << std::endl;
            } else {
                std::cout << std::endl;
            }
            continue;
        }

        std::cout << "ERROR" << std::endl;
    }

    return 0;
}
