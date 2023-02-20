#include <iostream>
#include <asio/experimental/as_tuple.hpp>
#include "Server.hpp"
#include "ExampleEnum.hpp"
#include "Message.hpp"
#include "ClientSession.hpp"

ClientSession::ClientSession(asio::ip::tcp::socket skt, Server* srv) :
	client(std::move(skt)),
	server(srv)
{
	isConnected = true;
	asio::co_spawn(server->GetProcessingContext(), WriteHeader(), asio::detached);
	std::cout << "Session created\n";
}
ClientSession::~ClientSession() {
	isConnected = false;
	//client.cancel();
	//client.close();
	std::cout << "Session terminated\n";
}
void ClientSession::PushMessage(const Message<ExampleEnum>& msg) {
	messages.push(msg);
}
bool ClientSession::IsConnected() const {
	return isConnected;
}
asio::ip::tcp::endpoint ClientSession::GetClientRemoteEndpoint() const {
	return client.remote_endpoint();
}
asio::awaitable<void> ClientSession::ReadHeader() {
	while (isConnected) {
		auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(&message.header, sizeof(MessageHeader<ExampleEnum>)), asio::experimental::as_tuple(asio::use_awaitable));
		if (errorReading) {
			std::cerr << errorReading.message() << "\n";
			//client.cancel();
			//client.close();
			isConnected = false;
			break;
		}
		if (message.header.bodySize > 0) {
			message.body.resize(message.header.bodySize);
			co_await ReadBody();
		}
		else {
			server->RegisterMessage(message, shared_from_this());
		}
	}
}
asio::awaitable<void> ClientSession::ReadBody() {
	auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(message.body.data(), message.header.bodySize), asio::experimental::as_tuple(asio::use_awaitable));
	if (errorReading) {
		std::cerr << errorReading.message() << "\n";
		//client.cancel();
		//client.close();
		isConnected = false;
	}
	else {
		server->RegisterMessage(message, shared_from_this());
	}
}
asio::awaitable<void> ClientSession::WriteHeader() {
	while (isConnected) {
		messages.wait();
		while (!messages.empty()) {
			std::cout << "Started writing header...\n";
			auto [error, n] = co_await asio::async_write(client, asio::buffer(&messages.front().header, sizeof(MessageHeader<ExampleEnum>)), asio::experimental::as_tuple(asio::use_awaitable));
			if (error) {
				std::cerr << error.message() << "\n";
				//client.cancel();
				//client.close();
				isConnected = false;
				break;
			}
			if (messages.front().header.bodySize > 0) {
				co_await WriteBody();
			}
			else {
				messages.pop();
			}
		}
	}
}
asio::awaitable<void> ClientSession::WriteBody() {
	std::cout << "Started writing body...\n";
	auto [error, n] = co_await asio::async_write(client, asio::buffer(messages.front().body.data(), messages.front().header.bodySize), asio::experimental::as_tuple(asio::use_awaitable));
	if (error) {
		std::cerr << error.message() << "\n";
		//client.cancel();
		//client.close();
		isConnected = false;
	}
	else {
		messages.pop();
	}
}