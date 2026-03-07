#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>
#include <filesystem>

const std::string CMD_SET  = "SET";
const std::string CMD_PUT  = "PUT";
const std::string CMD_GET  = "GET";
const std::string CMD_EXIT = "EXIT";

const std::string RESP_OK    = "OK";
const std::string RESP_ERROR = "ERROR";

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

bool parseSetLikeCommand(
    const std::string& line,
    std::string& cmdOut,
    std::string& keyOut,
    std::string& valueOut
) {
    std::istringstream iss(line);

    if (!(iss >> cmdOut)) {
        return false;
    }

    if (cmdOut != CMD_SET && cmdOut != CMD_PUT) {
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

std::filesystem::path getDatabasePath(const char* argv0) {
    std::filesystem::path exePath = std::filesystem::absolute(argv0);
    std::filesystem::path exeDir = exePath.parent_path();
    return exeDir / "data.db";
}

void replayLog(KVIndex& index, const std::filesystem::path& dbPath) {
    std::ifstream in(dbPath);

    if (!in.is_open()) {
        return;
    }

    std::string line;

    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        std::string cmd;
        std::string key;
        std::string value;

        if (parseSetLikeCommand(line, cmd, key, value)) {
            index.set(key, value);
        }
    }
}

bool appendToDisk(
    const std::filesystem::path& dbPath,
    const std::string& cmd,
    const std::string& key,
    const std::string& value
) {
    std::ofstream out(dbPath, std::ios::app);

    if (!out.is_open()) {
        return false;
    }

    out << cmd << " " << key;
    if (!value.empty()) {
        out << " " << value;
    }
    out << "\n";
    out.flush();

    return out.good();
}

void handleSetLike(
    KVIndex& index,
    const std::filesystem::path& dbPath,
    const std::string& cmd,
    const std::string& key,
    const std::string& value,
    bool printSetResponse
) {
    if (!appendToDisk(dbPath, cmd, key, value)) {
        std::cout << RESP_ERROR << ": failed to write to data.db" << std::endl;
        return;
    }

    index.set(key, value);

    if (printSetResponse) {
        std::cout << RESP_OK << std::endl;
    }
}

void handleGet(const KVIndex& index, const std::string& key) {
    std::string value;

    if (index.get(key, value)) {
        std::cout << value << std::endl;
    } else {
        std::cout << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::filesystem::path dbPath = getDatabasePath(argv[0]);

    KVIndex index;
    replayLog(index, dbPath);

    if (argc >= 2) {
        std::string cmd = argv[1];

        if ((cmd == CMD_SET || cmd == CMD_PUT) && argc >= 4) {
            std::string key = argv[2];
            std::string value = argv[3];

            for (int i = 4; i < argc; i++) {
                value += " ";
                value += argv[i];
            }

            handleSetLike(index, dbPath, cmd, key, value, false);
            return 0;
        }

        if (cmd == CMD_GET && argc >= 3) {
            std::string key = argv[2];
            handleGet(index, key);
            return 0;
        }

        if (cmd == CMD_EXIT) {
            return 0;
        }

        std::cout << RESP_ERROR << ": invalid command-line usage" << std::endl;
        return 0;
    }

    std::string line;

    while (std::getline(std::cin, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        if (line.empty()) {
            continue;
        }

        if (line == CMD_EXIT) {
            break;
        }

        std::string cmd;
        std::string key;
        std::string value;

        if (parseSetLikeCommand(line, cmd, key, value)) {
            handleSetLike(index, dbPath, cmd, key, value, true);
            continue;
        }

        if (line.rfind(CMD_GET + " ", 0) == 0) {
            std::istringstream iss(line);
            std::string getCmd;

            if (!(iss >> getCmd >> key) || getCmd != CMD_GET) {
                std::cout << RESP_ERROR << ": malformed GET command" << std::endl;
                continue;
            }

            handleGet(index, key);
            continue;
        }

        std::cout << RESP_ERROR << ": unknown command" << std::endl;
    }

    return 0;
}
