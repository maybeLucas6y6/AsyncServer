#include "Server.hpp"
#include <iostream>
#include <asio/experimental/as_tuple.hpp>

					  Server::Server(const char* address, asio::ip::port_type port) :
	listeningEndpoint(asio::ip::address::from_string(address), port),
	acceptor(listeningContext, listeningEndpoint),
	listeningThread([&] { listeningContext.run(); }),
	processingThread([&] { processingContext.run(); })
{
	asio::co_spawn(listeningContext, Listen(), asio::detached);
	asio::co_spawn(processingContext, Process(), asio::detached);
	std::cout << "Server started\n";
}
					  Server::~Server() {
	listeningContext.stop();
	processingContext.stop();

	std::cout << "Server stopped\n";
}
asio::awaitable<void> Server::Listen() {
	while (true) {
		auto [error, client] = co_await acceptor.async_accept(asio::experimental::as_tuple(asio::use_awaitable));
		if (error) {
			std::cerr << error.message();
		}
		else {
			// add code here to validate client connection
			std::cout << client.remote_endpoint() << " connected\n";

			auto newconn = std::make_shared<ClientSession>(std::move(client), this);
			clients.push_back(std::move(newconn));
			co_spawn(listeningContext, clients.back()->Start(), asio::detached); // should change this
		}
	}
}
asio::awaitable<void> Server::Process() {
	while (true) {
		messages.wait();
		while (!messages.empty()) {
			std::cout << "Message found in queue. Sending back...\n";
			auto& msg = messages.front();
			co_await MessageAllClients(msg.message);
			messages.pop();
		}
	}
}
asio::awaitable<void> Server::MessageClient(std::shared_ptr<ClientSession> session, std::string msg) {
	auto [error, sent] = co_await asio::async_write(session->client, asio::buffer(msg), asio::experimental::as_tuple(asio::use_awaitable));
	// handle error
}
asio::awaitable<void> Server::MessageAllClients(std::string msg) {
	for (auto& conn : clients) {
		co_await conn->Write(msg);
		//auto [e, n] = co_await async_write(conn->client, asio::buffer(msg, 5), asio::experimental::as_tuple(asio::use_awaitable));
		//std::cout << "Sent " << n << " bytes: " << msg << "---\n";
		// handle error
	}
}
asio::awaitable<void> Server::MessageAllClients(std::string msg, std::shared_ptr<ClientSession> except) {
	for (auto& conn : clients) {
		if (conn == except) {
			continue;
		}
		auto [e, n] = co_await async_write(conn->client, asio::buffer(msg), asio::experimental::as_tuple(asio::use_awaitable));
		// handle error
	}
}
void				  Server::RegisterMessage(std::shared_ptr<ClientSession> session, std::string msg) {
	messages.push({ session, msg });
}