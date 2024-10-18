#include "raft_node.h"
#include "config_parser.h"
#include <iostream>

int main() {
    std::vector<std::string> nodeIds = ConfigParser::parseConfig("../config/cluster_config.yaml");
    
    if (nodeIds.empty()) {
        std::cerr << "No nodes found in the configuration file." << std::endl;
        return 1;
    }
    
    boost::asio::io_context ioContext;

    std::vector<std::shared_ptr<RaftNode>> nodes;

    for (const auto& nodeId : nodeIds) {
        std::vector<std::string> peerIds;
        for (const auto& peerId : nodeIds) {
            if (peerId != nodeId) {
                peerIds.push_back(peerId);
            }
        }

        auto node = std::make_shared<RaftNode>(ioContext, nodeId, peerIds);
        nodes.push_back(node);
    }

    for (auto node : nodes) {
        node->initializePeerConnections();
    }

    std::thread t([&ioContext]() { ioContext.run(); });
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    nodes[0]->startElection();

    t.join();

    return 0;
}