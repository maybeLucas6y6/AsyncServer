#include "Server.hpp"
#include <iostream>
#include <asio/experimental/as_tuple.hpp>

Server::Server(const char* address, asio::ip::port_type port) :
	listeningEndpoint(asio::ip::address::from_string(address), port),
	acceptor(listeningContext, listeningEndpoint),
	listeningThread([&] { listeningContext.run(); })
	//processingThread([&] { processingContext.run(); })
{
	asio::co_spawn(listeningContext, Listen(), asio::detached);
	//asio::co_spawn(processingContext, Process(), asio::detached);
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
asio::awaitable<void> Server::Process() {
	while (true) {
		Sleep(750);
		Message<ExampleEnum> m;
		ExampleStruct s{ 10,98 };
		m << s;
		co_await MessageAllClients(std::move(m));
	}
	//while (true) {
	//	messages.wait();
	//	while (!messages.empty()) {
	//		std::cout << "Message found in queue. Sending back...\n";
	//		auto msg = messages.front();
	//		co_await MessageAllClients(msg.message);
	//		messages.pop();
	//	}
	//}
}
//asio::awaitable<void> Server::MessageClient(std::shared_ptr<ClientSession> session, std::string msg) {
//	co_await session->Write(msg);
//}
asio::awaitable<void> Server::MessageAllClients(Message<ExampleEnum> msg) {
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
		co_await conn->WriteHeader(msg);
	}
}
//asio::awaitable<void> Server::MessageAllClients(std::string msg, std::shared_ptr<ClientSession> except) {
//	for (auto& conn : clients) {
//		if (!conn->client.is_open()) {
//			clients.erase(conn);
//			continue;
//		}
//		if (conn == except) {
//			continue;
//		}
//		co_await conn->Write(msg);
//	}
//}
void Server::RegisterMessage(std::shared_ptr<ClientSession> session, Message<ExampleEnum> msg) {
	messages.push({ session, msg });
}