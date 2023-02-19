#include "Server.hpp"
#include <iostream>
#include <asio/experimental/as_tuple.hpp>

Server::Server(const char* address, asio::ip::port_type port) :
	listeningEndpoint(asio::ip::address::from_string(address), port),
	acceptor(listeningContext, listeningEndpoint),
	listeningThread([&] { listeningContext.run(); }),
	work(processingContext.get_executor()),
	processingThread([&] { processingContext.run(); })
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

			auto newconn = std::make_shared<ClientSession>(std::move(client), this);
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
		//std::cout << "Pr ctx is stopped: " << processingContext.stopped() << "\n";
		messages.wait();
		while (!messages.empty()) {
			//std::cout << "Clients: " << clients.size() << "\n";
			auto msg = messages.front();
			ExampleStruct s;
			msg.message >> s;
			std::cout << msg.session->client.remote_endpoint() << ": " << s.a << " " << s.b << "\n";
			MessageAllClients(messages.front().message);
			messages.pop();
		}
	}
}
void Server::MessageClient(Message<ExampleEnum> msg, std::shared_ptr<ClientSession> session) {
	session->messages.push(msg);
}
void Server::MessageAllClients(Message<ExampleEnum> msg) {
	std::set<std::shared_ptr<ClientSession>> offline;
	for (auto& conn : clients) {
		if (conn && !conn->client.is_open()) {
			offline.insert(conn);
			continue;
		}
		conn->messages.push(msg);
	}
	for (auto& conn : offline) {
		//clients.erase(conn);
	}
}
void Server::MessageAllClients(Message<ExampleEnum> msg, std::shared_ptr<ClientSession> except) {
	std::set<std::shared_ptr<ClientSession>> offline;
	for (auto& conn : clients) {
		if (conn && !conn->client.is_open()) {
			offline.insert(conn);
			continue;
		}
		if (conn == except) {
			continue;
		}
		conn->messages.push(msg);
	}
	for (auto& conn : offline) {
		std::cout << conn->client.remote_endpoint() << " disconnected\n";
		clients.erase(conn);
	}
}
void Server::RegisterMessage(std::shared_ptr<ClientSession> session, Message<ExampleEnum> msg) {
	messages.push({ session, msg });
}