#pragma once

#include <asio.hpp>
#include <thread>
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
	asio::executor_work_guard<decltype(processingContext.get_executor())> work;
	asio::ip::tcp::endpoint listeningEndpoint;
	asio::ip::tcp::acceptor acceptor;
	std::set<std::shared_ptr<ClientSession>> clients;
	MutexQueue<OwnedMessage> messagesReceived;
	asio::awaitable<void> Listen();
public:
	Server(const char* address, asio::ip::port_type port);
	~Server();
	asio::io_context& GetProcessingContext();
	void Process();
	void RegisterMessage(const Message<ExampleEnum>& msg, std::shared_ptr<ClientSession> session);
protected:
	void MessageClient(const Message<ExampleEnum>& msg, std::shared_ptr<ClientSession> session);
	void MessageAllClients(const Message<ExampleEnum>& msg);
	void MessageAllClientsExcept(const Message<ExampleEnum>& msg, std::shared_ptr<ClientSession> except);
};