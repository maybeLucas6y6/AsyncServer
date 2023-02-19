#pragma once

#include <thread>
#include <asio.hpp>
#include <set>
#include "ExampleEnum.hpp"
#include "ClientSession.hpp"
#include "MutexQueue.hpp"
#include "Message.hpp"
#include "OwnedMessage.hpp"

// TODO: on client disconnect
// TODO: check all shared_ptrs

class Server {
private:
	std::jthread listeningThread;
	asio::io_context listeningContext;
	std::jthread processingThread;
	asio::io_context processingContext;
	asio::ip::tcp::endpoint listeningEndpoint;
	asio::ip::tcp::acceptor acceptor;
	std::set<std::shared_ptr<ClientSession>> clients;
	MutexQueue<OwnedMessage> messages;
public:
	Server(const char* address, asio::ip::port_type port);
	~Server();
	asio::awaitable<void> Listen();
	void Process();
	void MessageClient(Message<ExampleEnum> msg, std::shared_ptr<ClientSession> session);
	void MessageAllClients(Message<ExampleEnum> msg);
	void MessageAllClients(Message<ExampleEnum> msg, std::shared_ptr<ClientSession> except);
	void RegisterMessage(std::shared_ptr<ClientSession> session, Message<ExampleEnum> msg);
};