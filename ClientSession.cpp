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
	isConnected = true;
	asio::co_spawn(server->processingContext, WriteHeader(), asio::detached);
	//server->processingContext.post(WriteHeader());
	std::cout << "Session created\n";
}
ClientSession::~ClientSession() {
	isConnected = false;
	//client.cancel();
	//client.close();
	std::cout << "Session terminated\n";
}
asio::awaitable<void> ClientSession::ReadHeader() {
	while (isConnected) {
		auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(&message.header, sizeof(MessageHeader<ExampleEnum>)), asio::experimental::as_tuple(asio::use_awaitable));
		std::cout << "Bytes read (header): " << bytesRead << "\n";
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
			server->RegisterMessage(shared_from_this(), message);
		}
	}
}
asio::awaitable<void> ClientSession::ReadBody() {
	auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(message.body.data(), message.header.bodySize), asio::experimental::as_tuple(asio::use_awaitable));
	std::cout << "Bytes read (body): " << bytesRead << "\n";
	if (errorReading) {
		std::cerr << errorReading.message() << "\n";
		//client.cancel();
		//client.close();
		isConnected = false;
	}
	else {
		server->RegisterMessage(shared_from_this(), message);
	}
}
asio::awaitable<void> ClientSession::WriteHeader() {
	//std::cout << "Started writing...\n";
	while (isConnected) {
		messages.wait();
		while (!messages.empty()) {
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
	auto [error, n] = co_await asio::async_write(client, asio::buffer(messages.front().body.data(), messages.front().header.bodySize), asio::experimental::as_tuple(asio::use_awaitable));
	if (error) {
		std::cerr << error.message() << "\n";
		//client.cancel();
		//client.close();
		isConnected = false;
	}
	else {
		auto msg = messages.front();
		ExampleStruct s;
		msg >> s;
		std::cout << "Sent: " << s.a << " " << s.b << " to: " << client.remote_endpoint() << "\n";
	}
	messages.pop();
}