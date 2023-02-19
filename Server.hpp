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
	//MutexQueue messagesOut;
public:
	Server(const char* address, asio::ip::port_type port);
	~Server();
	asio::awaitable<void> Listen();
	void Process();
	//asio::awaitable<void> MessageClient(std::shared_ptr<ClientSession> session, std::string msg);
	void MessageAllClients(Message<ExampleEnum> msg);
	//asio::awaitable<void> MessageAllClients(std::string msg, std::shared_ptr<ClientSession> except);
	void RegisterMessage(std::shared_ptr<ClientSession> session, Message<ExampleEnum> msg);
};