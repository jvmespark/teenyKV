#include "raft_node.h"
#include <boost/asio.hpp> 
#include <random>
#include <iostream>

// =============== LEADER ELECTION ===============

void RaftNode::startElection() {
    currentState = NodeState::CANDIDATE;
    currentTerm++;
    votedFor = std::stoi(nodeId);
    int votesReceived = 1;

    resetElectionTimeout();

    for (auto& peer : peerIds) {
        std::cout<<"Requesting vote for node "<<nodeId<<" from peer "<<peer<<std::endl;
        
        sendVoteRequest(currentTerm, std::stoi(peer));
    }
}

void RaftNode::sendVoteRequest(int term, int peerId) {
    std::string request = "vote_request:" + std::to_string(term) + ":" + std::to_string(peerId) + "\n";

    auto peerSocket = findPeerSocket(peerId);
    if (peerSocket) {
        boost::asio::async_write(*peerSocket, boost::asio::buffer(request),
            [this, peerId](const boost::system::error_code& ec, std::size_t /*length*/) {
                if (!ec) {
                    std::cout << "Vote request sent to peer " << peerId << std::endl;
                } else {
                    std::cerr << "Failed to send vote request to peer " << peerId << ": " << ec.message() << std::endl;
                }
            });
    } else {
        std::cerr << "No socket found for peer " << peerId << std::endl;
    }
}

void RaftNode::handleVoteRequest(int term, int peerId) {
    if (term > currentTerm) {
        currentTerm = term;
        votedFor = peerId;
        sendVoteResponse(peerId, true);  
    } else {
        sendVoteResponse(peerId, false); 
    }
}

void RaftNode::sendVoteResponse(int candidateId, bool granted) {
    std::string response = "vote_response:" + std::to_string((granted ? 1 : 0)) + "\n";

    auto socket = findPeerSocket(candidateId);
    if (!socket) {
        std::cerr << "Failed to find socket for candidate " << candidateId << std::endl;
        return;
    }

    auto responseBuffer = std::make_shared<std::string>(response);

    boost::asio::async_write(*socket, boost::asio::buffer(*responseBuffer), 
        [responseBuffer, candidateId](boost::system::error_code ec, std::size_t /*length*/) {
            if (!ec) {
                std::cout << "Sent vote response to candidate " << candidateId << std::endl;
            } else {
                std::cerr << "Failed to send vote response to candidate " << candidateId 
                          << ": " << ec.message() << std::endl;
            }
        }
    );
}

void RaftNode::handleVoteResponse(bool voteGranted) {
    if (voteGranted) {
        votesReceived++;
        std::cout << "Vote granted. Current votes: " << votesReceived << std::endl;
    }

    if (votesReceived > peerIds.size() / 2) {
        std::cout << "Node " << nodeId << " became the leader for term " << currentTerm << std::endl;
        becomeLeader();
    }
}

void RaftNode::sendHeartBeat() {
    if (currentState == NodeState::LEADER) {
        std::cout << "Node " << nodeId << " sending heartbeat for term " << currentTerm << std::endl;

        for (const auto& peer : peerIds) {
            std::string host = "localhost";  
            std::string port = peer;  

            try {
                boost::asio::ip::tcp::socket socket(ioContext);
                boost::asio::ip::tcp::resolver resolver(ioContext);
                auto endpoints = resolver.resolve(host, port);
                boost::asio::connect(socket, endpoints);

                std::string message = "heartbeat:" + std::to_string(currentTerm);

                boost::asio::write(socket, boost::asio::buffer(message));

                std::cout << "Sent heartbeat to peer " << peer << std::endl;
            }
            catch (std::exception& e) {
                std::cerr << "Failed to send heartbeat to peer " << peer << ": " << e.what() << std::endl;
            }
        }

        resetHeartBeatTimer();
    }
}

void RaftNode::resetElectionTimeout() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(150, 300);  

    electionTimer.expires_after(std::chrono::milliseconds(dis(gen)));
    electionTimer.async_wait([this](const boost::system::error_code& ec) {
        if (!ec) {
            startElection(); 
        }
    });
}

void RaftNode::resetHeartBeatTimer() {
    heartbeatTimer.expires_after(std::chrono::milliseconds(heartbeatInterval));
    heartbeatTimer.async_wait([this](const boost::system::error_code& ec) {
        if (!ec) {
            sendHeartBeat();  
        }
    });
}

void RaftNode::becomeLeader() {
    currentState = NodeState::LEADER;
    std::cout<<nodeId<<" has become the leader for term "<<currentTerm<<std::endl;
    sendHeartBeat();
}

void RaftNode::becomeFollower(int term) {
    if (term > currentTerm) {
        currentTerm = term;
        votedFor = -1;
    }
    currentState = NodeState::FOLLOWER;
    std::cout << "Node " << nodeId << " becomes a follower for term " << currentTerm << std::endl;
    
    resetElectionTimeout();
}

void RaftNode::handleHeartBeat(int term) {
    if (term > currentTerm) {
        becomeFollower(term);
    } else if (currentState != NodeState::FOLLOWER) {
        becomeFollower(currentTerm);
    }
    
    resetElectionTimeout();
    resetHeartBeatTimer();
}

void RaftNode::setupNetworking() {
    boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::tcp::v4(), std::stoi(nodeId) + 10000);
    boost::system::error_code ec;

    acceptor.open(endpoint.protocol());
    if (ec) {
        std::cerr << "Error opening acceptor for node " << nodeId << ": " << ec.message() << std::endl;
        return;
    }

    acceptor.set_option(boost::asio::ip::tcp::acceptor::reuse_address(true), ec);
    if (ec) {
        std::cerr << "Error setting SO_REUSEADDR for node " << nodeId << ": " << ec.message() << std::endl;
        return;
    }

    acceptor.bind(endpoint, ec);
    if (ec) {
        std::cerr << "Error binding acceptor for node " << nodeId << ": " << ec.message() << std::endl;
        return;
    }

    acceptor.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
        std::cerr << "Error listening on acceptor for node " << nodeId << ": " << ec.message() << std::endl;
        return;
    }

    std::cout << "Node " << nodeId << " is now set up on port " << endpoint.port() << " and ready to accept connections." << std::endl;

    startListening();
}

void RaftNode::initializePeerConnections() {
    for (const auto& peerId : peerIds) {
        auto socket = std::make_shared<boost::asio::ip::tcp::socket>(ioContext);
        boost::asio::ip::tcp::endpoint endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 10000 + std::stoi(peerId));
        
        boost::system::error_code ec;
        socket->connect(endpoint, ec);
        if (!ec) {
        
            std::string endpoint_str = "127.0.0.1:" + std::to_string(std::stoi(peerId) + 10000);
            peerEndpointToId[endpoint_str] = std::stoi(peerId); 
        
            peerSockets[std::stoi(peerId)] = socket;
            std::cout << "Connected to peer " << peerId << std::endl;
        } else {
            std::cerr << "Failed to connect to peer " << peerId << ": " << ec.message() << std::endl;
        }
    }
}

void RaftNode::startListening() {
    acceptNewConnection();
}

void RaftNode::acceptNewConnection() {
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(ioContext);
    acceptor.async_accept(*socket, [this, socket](const boost::system::error_code& error) {
        if (!error) {
            handleIncomingConnection(socket);
        }
        acceptNewConnection();
    });
}

void RaftNode::handleIncomingConnection(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    std::cout << "Received connection on node " << nodeId << std::endl;

    int peerId = getPeerIdFromSocket(*socket);
    peerSockets[peerId] = socket;

    boost::asio::streambuf buffer;

    try {
        boost::asio::read_until(*socket, buffer, "\n");

        std::istream is(&buffer);
        std::string message;
        std::getline(is, message);

        std::vector<std::string> parsed_values;
        std::stringstream ss(message);
        std::string segment;

        while (std::getline(ss, segment, ':')) {
            try {
                parsed_values.push_back(segment);
            } catch (const std::invalid_argument& e) {
                std::cerr << "Error parsing integer value: " << e.what() << std::endl;
            }
        }

        std::string command = parsed_values[0];

        if (command == "heartbeat") {
            int term = std::stoi(parsed_values[1]);

            handleHeartBeat(term);
        } else if (command == "vote_request") {
            int term = std::stoi(parsed_values[1]);
            int peerId = std::stoi(parsed_values[2]);

            handleVoteRequest(term, peerId);
        } else if (command == "vote_response") {
            bool voteGranted = bool(std::stoi(parsed_values[1]));

            handleVoteResponse(voteGranted);
        }
    } catch (const boost::system::system_error& e) {
        std::cerr << "Error on receiving connection: " << e.what() << std::endl;
    }
}

std::shared_ptr<boost::asio::ip::tcp::socket> RaftNode::findPeerSocket(int peerId) {
    auto it = peerSockets.find(peerId);
    if (it != peerSockets.end()) {
        return it->second;
    } else {
        std::cerr << "No socket found for peer " << peerId << " from "<< nodeId<< std::endl;
        return nullptr;
    }
}

int RaftNode::getPeerIdFromSocket(const boost::asio::ip::tcp::socket& socket) {
    boost::asio::ip::tcp::endpoint remoteEndpoint = socket.remote_endpoint();
    
    std::string endpoint = remoteEndpoint.address().to_string() + ":" + std::to_string(remoteEndpoint.port());

    auto it = peerEndpointToId.find(endpoint);
    if (it != peerEndpointToId.end()) {
        return it->second;
    } else {
        std::cerr << "No peer ID found for endpoint " << endpoint << std::endl;
        return -1;
    }
}


// =============== LOG REPLICATION ===============

/*
void RaftNode::replicateLogEntry(std::string entry) {
    if (currentState != NodeState::LEADER)
        return;
    for (auto& peer : peerIds) {
        std::cout<<"Replicating log entry to "<<peer<<" : "<<entry<<std::endl;
    }
}

bool RaftNode::put(const std::string& key, const std::string& value) {
    if (currentState == NodeState::LEADER) {
        kvStore.put(key, value);
        replicateLogEntry("PUT " + key + " " + value);
        return true;
    }
    return false;
}

bool RaftNode::get(const std::string& key, std::string& value) {
    return kvStore.get(key, value);
}

bool RaftNode::del(const std::string& key) {
    if (currentState == NodeState::LEADER) {
        kvStore.del(key);
        replicateLogEntry("DEL " + key);
        return true;
    }
    return false;
}

bool RaftNode::append(const std::string& key, const std::string& value) {
    if (currentState == NodeState::LEADER) {
        kvStore.append(key, value);
        replicateLogEntry("APPEND " + key + " " + value);
        return true;
    }
    return false;
}
*/