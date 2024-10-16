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

    // Initialize all nodes
    for (const auto& nodeId : nodeIds) {
        std::vector<std::string> peerIds;
        for (const auto& peerId : nodeIds) {
            if (peerId != nodeId) {
                peerIds.push_back(peerId);
            }
        }

        // Instantiate a RaftNode with its ID and its peer IDs
        auto node = std::make_shared<RaftNode>(ioContext, nodeId, peerIds);

        // Add the node to the list
        nodes.push_back(node);
    }

    // Start the io_context in a separate thread to allow asynchronous operations
    std::thread t([&ioContext]() { ioContext.run(); });

    // Start an election for one of the nodes
    nodes[0]->startElection();

    // Wait for the thread to finish
    t.join();

    return 0;
}