#include "ConfigManager.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <fstream>
#include <sstream>

bool ConfigManager::loadConfig(const std::string& filename) {
    configMap.clear();
    
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false; // 文件不存在，使用默认值
    }
    
    std::string line;
    while (std::getline(file, line)) {
        parseLine(line);
    }
    
    file.close();
    return true;
}

bool ConfigManager::saveConfig(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }
    
    for (const auto& pair : configMap) {
        file << pair.first << "=" << pair.second << "\n";
    }
    
    file.close();
    return true;
}

std::string ConfigManager::getString(const std::string& key, const std::string& defaultValue) const {
    auto it = configMap.find(key);
    if (it != configMap.end()) {
        return it->second;
    }
    return defaultValue;
}

int ConfigManager::getInt(const std::string& key, int defaultValue) const {
    auto it = configMap.find(key);
    if (it != configMap.end()) {
        try {
            return std::stoi(it->second);
        } catch (...) {
            return defaultValue;
        }
    }
    return defaultValue;
}

char ConfigManager::getChar(const std::string& key, char defaultValue) const {
    auto it = configMap.find(key);
    if (it != configMap.end() && !it->second.empty()) {
        return it->second[0];
    }
    return defaultValue;
}

void ConfigManager::setString(const std::string& key, const std::string& value) {
    configMap[key] = value;
}

void ConfigManager::setInt(const std::string& key, int value) {
    configMap[key] = std::to_string(value);
}

void ConfigManager::setChar(const std::string& key, char value) {
    configMap[key] = std::string(1, value);
}

std::string ConfigManager::trim(const std::string& str) const {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return "";
    }
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

void ConfigManager::parseLine(const std::string& line) {
    // 跳过注释和空行
    std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed[0] == '#' || trimmed[0] == ';') {
        return;
    }
    
    // 查找等号
    size_t pos = trimmed.find('=');
    if (pos == std::string::npos) {
        return;
    }
    
    std::string key = trim(trimmed.substr(0, pos));
    std::string value = trim(trimmed.substr(pos + 1));
    
    if (!key.empty()) {
        configMap[key] = value;
    }
}

