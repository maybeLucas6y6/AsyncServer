#pragma once

#include "Utilities.hpp"
#include "ClientSession.hpp"
#include "OwnedMessage.hpp"

class Server {
private:
	std::jthread listeningThread;
	asio::io_context listeningContext;

	std::jthread processingThread;
	asio::io_context processingContext;

	asio::ip::tcp::endpoint listeningEndpoint;
	asio::ip::tcp::acceptor acceptor;

	int id = 0;
	std::vector<std::shared_ptr<ClientSession>> clients; // it's shared_ptr because you can't copy/duplicate sockets
	std::queue<OwnedMessage> messages;
public:
	Server(const char* address, asio::ip::port_type port) :
		listeningEndpoint(asio::ip::address::from_string(address), port),
		acceptor(listeningContext, listeningEndpoint),
		listeningThread([&]{ listeningContext.run(); }),
		processingThread([&]{ processingContext.run(); }) 
	{
		asio::co_spawn(listeningContext, Listen(), asio::detached);
		asio::co_spawn(processingContext, Process(), asio::detached);
	}
	~Server() {
		listeningContext.stop();
		processingContext.stop();

		std::cout << "Stopped!\n";
	}
	asio::awaitable<void> Listen() {
		while (true) {
			auto [error, client] = co_await acceptor.async_accept(use_nothrow_awaitable);
			if (error) {
				std::cerr << error.message();
			}
			else {
				auto newconn = std::make_shared<ClientSession>(std::move(client), id++, &messages);
				clients.push_back(std::move(newconn));
				co_spawn(listeningContext, clients.back()->start(), asio::detached); // should change this
				auto refused = RefuseConnection(clients.back()); // and this
				if (refused) {
					std::cout << "Client connection refused\n";
				}
				else {
					std::cout << "Client connection accepted\n";
				}
			}
		}
	}
	bool RefuseConnection(std::shared_ptr<ClientSession> session) {
		return false;
	}
	asio::awaitable<void> Process() {
		while (true) {
			while (messages.empty()) {

			}
			// take the first message in queue and send it to all clients except the one who sent it
			for (auto& conn : clients) {
				if (conn->id == messages.front().id) { // currently uses an int id, should update to pointer id or smth better
					continue;
				}
				auto [e, n] = co_await async_write(conn->client, asio::buffer(messages.front().message), use_nothrow_awaitable);
			}
			messages.pop();
		}
	}
};