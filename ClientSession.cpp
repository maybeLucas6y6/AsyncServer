#include "ExampleEnum.hpp"
#include "Message.hpp"
#include "ClientSession.hpp"
#include "Server.hpp"
#include <asio/experimental/as_tuple.hpp>
#include <iostream>

#include <cstdint>
#include <vector>

ClientSession::ClientSession(asio::ip::tcp::socket skt, Server* srv) :
	client(std::move(skt)),
	server(srv)
{
	asio::co_spawn(server->processingContext, WriteHeader(), asio::detached);
	std::cout << "Session created\n";
}
ClientSession::~ClientSession() {
	std::cout << "Session terminated\n";
}
asio::awaitable<void> ClientSession::ReadHeader() {
	while (true) {
		auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(&message.header, sizeof(MessageHeader<ExampleEnum>)), asio::experimental::as_tuple(asio::use_awaitable));
		if (errorReading) {
			std::cerr << errorReading.message() << "\n";
			//client.cancel();
			//client.close();
			break;
		}
		else {
			if (message.header.bodySize > 0) {
				message.body.resize(message.header.bodySize);
				co_await ReadBody();
			}
			else {
				server->RegisterMessage(shared_from_this(), message);
			}
		}
	}
}
asio::awaitable<void> ClientSession::ReadBody() {
	auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(message.body.data(), message.header.bodySize), asio::experimental::as_tuple(asio::use_awaitable));
	if (errorReading) {
		std::cerr << errorReading.message() << "\n";
		//client.cancel();
		//client.close();
	}
	else {
		server->RegisterMessage(shared_from_this(), message);
	}
}
asio::awaitable<void> ClientSession::WriteHeader() {
	//std::cout << "Started writing...\n";
	while (true) {
		messages.wait();
		while (!messages.empty()) {
			auto [error, n] = co_await asio::async_write(client, asio::buffer(&messages.front().header, sizeof(MessageHeader<ExampleEnum>)), asio::experimental::as_tuple(asio::use_awaitable));
			if (error) {
				std::cerr << error.message() << "\n";
				//client.cancel();
				//client.close();
				break;
			}
			else {
				if (messages.front().header.bodySize > 0) {
					co_await WriteBody();
				}
				else {
					messages.pop();
				}
			}
		}
	}
}
asio::awaitable<void> ClientSession::WriteBody() {
	auto [error, n] = co_await asio::async_write(client, asio::buffer(messages.front().body.data(), messages.front().header.bodySize), asio::experimental::as_tuple(asio::use_awaitable));
	if (error) {
		std::cerr << error.message() << "\n";
		//client.cancel();
		//client.close();
	}
	else {
		//std::cout << "Sent " << n << " bytes\n";
	}
	messages.pop();
}