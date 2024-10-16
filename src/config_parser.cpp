#include "config_parser.h"
#include <yaml-cpp/yaml.h>
#include <iostream>
#include <vector>
#include <string>

std::vector<std::string> ConfigParser::parseConfig(const std::string& filePath) {
    std::vector<std::string> nodeIds;
    
    try {
        YAML::Node config = YAML::LoadFile(filePath);

        if (config["nodes"] && config["nodes"].IsSequence()) {
            for (auto node : config["nodes"]) {
                nodeIds.push_back(node.as<std::string>());
            }
        } else {
            std::cerr << "Config file error: 'nodes' entry is missing or not a sequence." << std::endl;
        }
    }
    catch (const YAML::BadFile& e) {
        std::cerr << "Error loading config file: " << e.what() << std::endl;
    }
    catch (const YAML::Exception& e) {
        std::cerr << "YAML parsing error: " << e.what() << std::endl;
    }

    return nodeIds;
}
