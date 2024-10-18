#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <boost/asio.hpp>
#include "key_value_store.h"

class RaftNode {
    public:
        enum NodeState {FOLLOWER, CANDIDATE, LEADER};

        RaftNode(boost::asio::io_context& ioContext, const std::string& id, const std::vector<std::string>& peers)
                : ioContext(ioContext), nodeId(id), peerIds(peers), currentTerm(0), votedFor(-1),
                  currentState(NodeState::FOLLOWER), acceptor(ioContext), heartbeatTimer(ioContext), electionTimer(ioContext) {
                    setupNetworking();
                    initializePeerConnections();
                  }

        // disable copying (for io_context)
        RaftNode(const RaftNode&) = delete;
        RaftNode& operator=(const RaftNode&) = delete;

        // default move semantics (for io_context)
        RaftNode(RaftNode&&) = default;
        RaftNode& operator=(RaftNode&&) = default;

        // =============== LEADER ELECTION ===============
        void startElection();
        void sendHeartBeat();
        void handleVoteRequest(int term, int peerId);
        void handleVoteResponse(bool voteGranted);
        void resetElectionTimeout();
        void resetHeartBeatTimer();
        void initializePeerConnections();

        // =============== LOG REPLICATION ===============
        /*
        void replicateLogEntry(std::string entry);
        bool put(const std::string& key, const std::string& value);
        bool get(const std::string& key, std::string& value);
        bool del(const std::string& key);
        bool append(const std::string& key, const std::string& value);
        */

    private:
        std::string nodeId;
        std::vector<std::string> peerIds;
        std::unordered_map<int, std::shared_ptr<boost::asio::ip::tcp::socket>> peerSockets;
        std::unordered_map<std::string, int> peerEndpointToId;
        NodeState currentState;
        int currentTerm;
        int votedFor;
        int votesReceived ;
        int electionTimeout;
        int heartbeatInterval;

        boost::asio::io_context& ioContext;
        boost::asio::steady_timer electionTimer;
        boost::asio::steady_timer heartbeatTimer;
        boost::asio::ip::tcp::acceptor acceptor;

        void becomeLeader();
        void becomeFollower(int term);
        void handleHeartBeat(int term);
        void setupNetworking();
        void startListening();
        void acceptNewConnection();
        void handleIncomingConnection(std::shared_ptr<boost::asio::ip::tcp::socket> socket);
        int getPeerIdFromSocket(const boost::asio::ip::tcp::socket& socket);
        std::shared_ptr<boost::asio::ip::tcp::socket> findPeerSocket(int peerId);
        void sendVoteRequest(int term, int peerId);
        void sendVoteResponse(int candidateId, bool granted);

        //KeyValueStore kvStore;
};