#pragma once
#include <string>
#include <vector>

class ConfigParser {
    public:
        static std::vector<std::string> parseConfig(const std::string& filePath);
};