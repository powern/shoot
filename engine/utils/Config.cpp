#include <fstream>
#include <sstream>
#include "Config.h"
#include "Log.h"

bool Config::load(const std::string &filename) {
    _filename = filename;
    _values.clear();

    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::string line;
    while (std::getline(file, line)) {
        // Strip comments and trim
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        // Find '=' separator
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;

        std::string key = line.substr(0, eqPos);
        std::string val = line.substr(eqPos + 1);

        // Trim whitespace
        auto trim = [](std::string &s) {
            size_t start = s.find_first_not_of(" \t\r\n");
            size_t end = s.find_last_not_of(" \t\r\n");
            if (start == std::string::npos) { s.clear(); return; }
            s = s.substr(start, end - start + 1);
        };
        trim(key);
        trim(val);

        if (key.empty()) continue;

        _values[key] = val;
    }

    Log::log("Config::load(): loaded " + std::to_string(_values.size()) + " keys from '" + filename + "'");
    return true;
}

void Config::save(const std::string &filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    for (auto &[key, val] : _values) {
        file << key << " = " << val << "\n";
    }
}

void Config::saveDefaults(const std::string &filename, const std::map<std::string, std::string> &defaults) {
    std::ofstream file(filename);
    if (!file.is_open()) return;

    for (auto &[key, val] : defaults) {
        file << key << " = " << val << "\n";
    }
}
