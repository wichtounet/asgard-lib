//=======================================================================
// Copyright (c) 2015-2016 Baptiste Wicht
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://opensource.org/licenses/MIT)
//=======================================================================

namespace asgard {

const std::size_t UNIX_PATH_MAX = 108;
const std::size_t buffer_size = 4096;

struct KeyValue {
    std::string key;
    std::string value;
};

inline void load_config(std::vector<KeyValue>& config) {
    std::ifstream file("/etc/asgard/conf");
    std::string str;
    std::size_t i = 0;

    while (std::getline(file, str)) {
        std::istringstream line(str);
        config.push_back(KeyValue());
        if (std::getline(line, config[i].key, '=')) {
            if (std::getline(line, config[i].value)) {
                i++;
            }
        }
    }
}

inline std::string get_string_value(std::vector<KeyValue>& config, const std::string& key) {
    for (std::size_t i = 0; i < config.size(); ++i) {
        if (config[i].key == key) {
            return config[i].value;
        }
    }
    std::cout << "Key not found !" << std::endl;
    return "";
}

inline int get_int_value(std::vector<KeyValue>& config, const std::string& key) {
    for (std::size_t i = 0; i < config.size(); ++i) {
        if (config[i].key == key) {
            return std::stoi(config[i].value);
        }
    }
    std::cout << "Key not found !" << std::endl;
    return -1;
}

} //end of namespace asgard
