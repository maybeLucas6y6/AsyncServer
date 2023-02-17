#pragma once

#include <thread>
#include <asio.hpp>
#include "ClientSession.hpp"
#include "MutexQueue.hpp"

class Server {
private:
	std::jthread listeningThread;
	asio::io_context listeningContext;

	std::jthread processingThread;
	asio::io_context processingContext;

	asio::ip::tcp::endpoint listeningEndpoint;
	asio::ip::tcp::acceptor acceptor;

	std::vector<std::shared_ptr<ClientSession>> clients; // it's shared_ptr because you can't copy/duplicate sockets
	MutexQueue messages;
public:
	Server(const char* address, asio::ip::port_type port);
	~Server();
	asio::awaitable<void> Listen();
	asio::awaitable<void> Process();
	asio::awaitable<void> MessageClient(std::shared_ptr<ClientSession> session, std::string msg);
	asio::awaitable<void> MessageAllClients(std::string msg);
	asio::awaitable<void> MessageAllClients(std::string msg, std::shared_ptr<ClientSession> except);
	void RegisterMessage(std::shared_ptr<ClientSession> session, std::string msg);
};