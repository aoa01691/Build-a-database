#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <sstream>

/*
    Simple persistent key-value store.

    Supported commands:
    - SET <key> <value>
    - PUT <key> <value>
    - GET <key>
    - EXIT

    Notes:
    - Data is persisted to data.db using append-only writes.
    - On startup, the log file is replayed to rebuild in-memory state.
    - A linear vector-based index is used instead of a hash map because
      the project explicitly forbids built-in dictionary/map types.
*/

// -------------------------------
// Constants
// -------------------------------
const std::string CMD_SET  = "SET";
const std::string CMD_PUT  = "PUT";
const std::string CMD_GET  = "GET";
const std::string CMD_EXIT = "EXIT";

const std::string RESP_OK    = "OK";
const std::string RESP_ERROR = "ERROR";

const char* DB_FILE = "data.db";

// -------------------------------
// Data structures
// -------------------------------
struct Entry {
    std::string key;
    std::string value;
};

/*
    In-memory index for key-value pairs.

    This uses a vector and linear search instead of std::map / std::unordered_map
    to satisfy the assignment requirement not to rely on built-in dictionary/map types.
*/
class KVIndex {
private:
    std::vector<Entry> entries;

public:
    /*
        Find the index of a key in the vector.
        Returns -1 if the key is not found.
    */
    int findKey(const std::string& key) const {
        for (int i = 0; i < static_cast<int>(entries.size()); i++) {
            if (entries[i].key == key) {
                return i;
            }
        }
        return -1;
    }

    /*
        Insert or overwrite a key-value pair.
        Last write wins.
    */
    void set(const std::string& key, const std::string& value) {
        int idx = findKey(key);

        if (idx == -1) {
            entries.push_back({key, value});
        } else {
            entries[idx].value = value;
        }
    }

    /*
        Retrieve a value by key.
        Returns true if found, false otherwise.
    */
    bool get(const std::string& key, std::string& outValue) const {
        int idx = findKey(key);

        if (idx == -1) {
            return false;
        }

        outValue = entries[idx].value;
        return true;
    }
};

// -------------------------------
// Parsing helpers
// -------------------------------

/*
    Parse a SET-like command line:
    SET <key> <value>
    PUT <key> <value>

    Returns true on success and fills cmdOut, keyOut, valueOut.
*/
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

// -------------------------------
// Persistence helpers
// -------------------------------

/*
    Replay the append-only log from disk into memory.

    Every valid SET/PUT line found in data.db is applied to the index.
    Since writes are replayed in order, last write wins naturally.
*/
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

        std::string cmd;
        std::string key;
        std::string value;

        if (parseSetLikeCommand(line, cmd, key, value)) {
            index.set(key, value);
        }
    }
}

/*
    Append a SET/PUT command to disk immediately.

    Returns true if the write succeeds, false otherwise.
*/
bool appendToDisk(
    const std::string& cmd,
    const std::string& key,
    const std::string& value
) {
    std::ofstream out(DB_FILE, std::ios::app);

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

// -------------------------------
// Command handlers
// -------------------------------

/*
    Handle SET/PUT.

    printSetResponse:
    - true  -> print OK / ERROR (used for interactive/stdin mode)
    - false -> suppress OK on success (useful if tester expects quiet CLI set mode)
*/
void handleSetLike(
    KVIndex& index,
    const std::string& cmd,
    const std::string& key,
    const std::string& value,
    bool printSetResponse
) {
    if (!appendToDisk(cmd, key, value)) {
        std::cout << RESP_ERROR << std::endl;
        return;
    }

    index.set(key, value);

    if (printSetResponse) {
        std::cout << RESP_OK << std::endl;
    }
}

/*
    Handle GET.

    If key exists, print the value.
    If not, print an empty line.
*/
void handleGet(const KVIndex& index, const std::string& key) {
    std::string value;

    if (index.get(key, value)) {
        std::cout << value << std::endl;
    } else {
        std::cout << std::endl;
    }
}

// -------------------------------
// Main
// -------------------------------

int main(int argc, char* argv[]) {
    KVIndex index;
    replayLog(index);

    /*
        Command-line mode:
        kvstore.exe SET key value
        kvstore.exe PUT key value
        kvstore.exe GET key
        kvstore.exe EXIT
    */
    if (argc >= 2) {
        std::string cmd = argv[1];

        if ((cmd == CMD_SET || cmd == CMD_PUT) && argc >= 4) {
            std::string key = argv[2];
            std::string value = argv[3];

            for (int i = 4; i < argc; i++) {
                value += " ";
                value += argv[i];
            }

            handleSetLike(index, cmd, key, value, false);
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

    /*
        Interactive / piped stdin mode
    */
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
            handleSetLike(index, cmd, key, value, true);
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
