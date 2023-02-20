#pragma once

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define ASIO_STANDALONE
#include <asio.hpp>
#include <set>
#include <thread>
#include "ExampleEnum.hpp"
#include "ClientSession.hpp"
#include "MutexQueue.hpp"
#include "Message.hpp"
#include "OwnedMessage.hpp"

// TODO: on client disconnect
// TODO: check all shared_ptrs

class Server {
public:
	asio::io_context processingContext;
	asio::executor_work_guard<decltype(processingContext.get_executor())> work; // move this
private:
	std::jthread listeningThread;
	asio::io_context listeningContext;
	std::jthread processingThread;
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
	void MessageAllClients(const Message<ExampleEnum>& msg);
	void MessageAllClients(Message<ExampleEnum> msg, std::shared_ptr<ClientSession> except);
	void RegisterMessage(std::shared_ptr<ClientSession> session, Message<ExampleEnum> msg);
};