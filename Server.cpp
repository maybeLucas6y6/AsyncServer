#include "Server.hpp"
#include <iostream>
#include <asio/experimental/as_tuple.hpp>

Server::Server(const char* address, asio::ip::port_type port) :
	listeningEndpoint(asio::ip::address::from_string(address), port),
	acceptor(listeningContext, listeningEndpoint),
	listeningThread([&] { listeningContext.run(); }),
	processingThread([&] { asio::executor_work_guard<decltype(processingContext.get_executor())> work{processingContext.get_executor()}; processingContext.run(); })
{
	asio::co_spawn(listeningContext, Listen(), asio::detached);
	std::cout << "Server started\n";
}
Server::~Server() {
	listeningContext.stop();
	processingContext.stop();
	acceptor.close();
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

			auto newconn = std::make_shared<ClientSession>(processingContext, std::move(client), this);
			co_spawn(listeningContext, newconn->ReadHeader(), asio::detached); // should change this
			clients.insert(std::move(newconn));
		}
	}
}
void Server::Process() {
	/*while (true) {
		Sleep(750);
		Message<ExampleEnum> m;
		ExampleStruct s{ 10,98 };
		m << s;
		MessageAllClients(std::move(m));
	}*/
	while (true) {
		messages.wait();
		while (!messages.empty()) {
			std::cout << "Message found in queue. Sending back...\n";
			auto msg = messages.front();
			MessageAllClients(std::move(msg.message));
			messages.pop();
		}
	}
}
void Server::MessageClient(Message<ExampleEnum> msg, std::shared_ptr<ClientSession> session) {
	session->messages.push(msg);
}
void Server::MessageAllClients(Message<ExampleEnum> msg) {
	bool clear = true;
	do {
		clear = true;
		for (auto& conn : clients) {
			if (!conn->client.is_open()) {
				clients.erase(conn);
				clear = false;
				break;
			}
		}
	} while (!clear);
	for (auto& conn : clients) {
		conn->messages.push(msg);
	}
}
void Server::MessageAllClients(Message<ExampleEnum> msg, std::shared_ptr<ClientSession> except) {
	bool clear = true;
	do {
		clear = true;
		for (auto& conn : clients) {
			if (!conn->client.is_open()) {
				clients.erase(conn);
				clear = false;
				break;
			}
		}
	} while (!clear);
	for (auto& conn : clients) {
		if (conn == except) {
			continue;
		}
		conn->messages.push(msg);
	}
}
void Server::RegisterMessage(std::shared_ptr<ClientSession> session, Message<ExampleEnum> msg) {
	messages.push({ session, msg });
}