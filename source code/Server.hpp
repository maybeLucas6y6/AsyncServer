#pragma once

#include "Utilities.hpp"
#include "ClientSession.hpp"
#include "OwnedMessage.hpp"
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
	MutexQueue<OwnedMessage> messages;
public:
	Server(const char* address, asio::ip::port_type port) :
		listeningEndpoint(asio::ip::address::from_string(address), port),
		acceptor(listeningContext, listeningEndpoint),
		listeningThread([&]{ listeningContext.run(); }),
		processingThread([&]{ processingContext.run(); }) 
	{
		asio::co_spawn(listeningContext, Listen(), asio::detached);
		asio::co_spawn(processingContext, Process(), asio::detached);
		std::cout << "Server started\n";
	}
	~Server() {
		listeningContext.stop();
		processingContext.stop();

		std::cout << "Server stopped\n";
	}
	asio::awaitable<void> Listen() {
		while (true) {
			auto [error, client] = co_await acceptor.async_accept(use_nothrow_awaitable);
			if (error) {
				std::cerr << error.message();
			}
			else {
				// add code here to validate client connection
				std::cout << client.remote_endpoint() << " connected\n";

				auto newconn = std::make_shared<ClientSession>(std::move(client), &messages);
				clients.push_back(std::move(newconn));
				co_spawn(listeningContext, clients.back()->start(), asio::detached); // should change this
			}
		}
	}
	asio::awaitable<void> Process() {
		while (true) {
			if (messages.empty()) {
				continue;
			}

			auto& msg = messages.front();
			co_await MessageAllClients(msg.message, msg.session);
			messages.pop();
		}
	}
	asio::awaitable<void> MessageClient(std::shared_ptr<ClientSession> session, std::string_view msg) {
		auto [error, sent] = co_await asio::async_write(session->client, asio::buffer(msg), use_nothrow_awaitable);
		// handle error
	}
	asio::awaitable<void> MessageAllClients(std::string_view msg) {
		for (auto& conn : clients) {
			auto [e, n] = co_await async_write(conn->client, asio::buffer(msg), use_nothrow_awaitable);
			// handle error
		}
	}
	asio::awaitable<void> MessageAllClients(std::string_view msg, std::shared_ptr<ClientSession> except) {
		for (auto& conn : clients) {
			if (conn == except) {
				continue;
			}
			auto [e, n] = co_await async_write(conn->client, asio::buffer(msg), use_nothrow_awaitable);
			// handle error
		}
	}
};