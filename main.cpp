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
        for (int i = 0; i < entries.size(); i++) {
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

    bool get(const std::string& key, std::string& value) const {
        int idx = findKey(key);

        if (idx == -1) {
            return false;
        }

        value = entries[idx].value;
        return true;
    }
};

static const char* DB_FILE = "data.db";

bool parseSetLine(const std::string& line, std::string& key, std::string& value) {
    std::istringstream iss(line);

    std::string cmd;

    if (!(iss >> cmd)) {
        return false;
    }

    if (cmd != "SET" && cmd != "PUT") {
        return false;
    }

    if (!(iss >> key)) {
        return false;
    }

    std::getline(iss, value);

    if (!value.empty() && value[0] == ' ') {
        value.erase(0, 1);
    }

    return true;
}

void replayLog(KVIndex& index) {

    std::ifstream file(DB_FILE);

    if (!file.is_open()) {
        return;
    }

    std::string line;

    while (std::getline(file, line)) {

        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string key;
        std::string value;

        if (parseSetLine(line, key, value)) {
            index.set(key, value);
        }
    }
}

bool appendToDisk(const std::string& command, const std::string& key, const std::string& value) {

    std::ofstream file(DB_FILE, std::ios::app);

    if (!file.is_open()) {
        return false;
    }

    file << command << " " << key;

    if (!value.empty()) {
        file << " " << value;
    }

    file << "\n";

    file.flush();

    return file.good();
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

        if (line.rfind("SET ",0) == 0 || line.rfind("PUT ",0) == 0) {

            std::string key;
            std::string value;

            if (!parseSetLine(line, key, value)) {
                std::cout << "ERROR\n";
                continue;
            }

            std::string command = line.substr(0,3);

            if (!appendToDisk(command, key, value)) {
                std::cout << "ERROR\n";
                continue;
            }

            index.set(key,value);

            std::cout << "OK\n";

            continue;
        }

        if (line.rfind("GET ",0) == 0) {

            std::istringstream iss(line);

            std::string cmd;
            std::string key;

            iss >> cmd >> key;

            std::string value;

            if (index.get(key,value)) {
                std::cout << value << "\n";
            } else {
                std::cout << "\n";
            }

            continue;
        }

        std::cout << "ERROR\n";
    }

    return 0;
}
