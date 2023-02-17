#include "ClientSession.hpp"
#include "Server.hpp"
#include <asio/experimental/as_tuple.hpp>
#include <iostream>

					  ClientSession::ClientSession(asio::ip::tcp::socket skt, Server* srv) :
	client(std::move(skt)),
	server(srv)
{

}
asio::awaitable<void> ClientSession::Start() {
	while (true) {
		char data[5] = { 0 };
		auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(data, 5), asio::experimental::as_tuple(asio::use_awaitable));
		if (errorReading) {
			//std::cerr << errorReading.message();
			std::cout << client.remote_endpoint() << " disconnected\n";
			break;
		}
		else {
			std::string msg = data;
			std::cout << "Received " << bytesRead << " bytes: " << msg << "\n";
			server->RegisterMessage(shared_from_this(), msg);
		}
	}
	client.close();
}
asio::awaitable<void> ClientSession::Write(std::string msg) {
	auto [errorWriting, bytesWritten] = co_await async_write(client, asio::buffer(msg, sizeof(msg)), asio::experimental::as_tuple(asio::use_awaitable));
	if (errorWriting) {
		//std::cerr << errorWriting.message();
		std::cout << client.remote_endpoint() << " disconnected\n";
		client.close();
	}
}