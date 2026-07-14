#ifndef ENGINE_CONFIG_H
#define ENGINE_CONFIG_H

#include <string>
#include <map>
#include <sstream>

class Config final {
private:
    std::map<std::string, std::string> _values;
    std::string _filename;

public:
    Config() = default;

    bool load(const std::string &filename);
    void save(const std::string &filename);
    void saveDefaults(const std::string &filename, const std::map<std::string, std::string> &defaults);

    bool has(const std::string &key) const;

    template<typename T>
    T get(const std::string &key, const T &defaultValue = T()) const {
        auto it = _values.find(key);
        if (it == _values.end()) return defaultValue;
        std::stringstream ss(it->second);
        T val;
        ss >> val;
        return val;
    }

    void set(const std::string &key, const std::string &value) { _values[key] = value; }
};

#endif
