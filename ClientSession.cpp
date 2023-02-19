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
	message.body.resize(32);
}
asio::awaitable<void> ClientSession::ReadHeader() {
	while (true) {
		auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(&message.header, sizeof(MessageHeader<ExampleEnum>)), asio::experimental::as_tuple(asio::use_awaitable));
		if (errorReading) {
			//std::cerr << errorReading.message();
			std::cout << client.remote_endpoint() << " disconnected\n";
			client.close();
		}
		else {
			if (message.header.bodySize > 0) {
				message.body.resize(message.header.bodySize); // this should be changed
				co_await ReadBody();
			}
			else {
				std::cout << "Received " << bytesRead << " bytes: " << message << "\n";
				server->RegisterMessage(shared_from_this(), message);
			}
		}
	}
}
asio::awaitable<void> ClientSession::ReadBody() {
	auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(message.body.data(), message.header.bodySize), asio::experimental::as_tuple(asio::use_awaitable));
	if (errorReading) {
		//std::cerr << errorReading.message();
		std::cout << client.remote_endpoint() << " disconnected\n";
		client.close();
	}
	else {
		std::cout << "Received " << bytesRead << " bytes: " << message << "\n";
		ExampleStruct s{0,0};
		//message >> s;
		std::vector<uint8_t> v = message.body;
		std::memcpy(&s, v.data(), sizeof(ExampleStruct));
		std::cout << "Contents: " << s.a << " " << s.b << "\n";
		server->RegisterMessage(shared_from_this(), message);
	}
}
asio::awaitable<void> ClientSession::WriteHeader(Message<ExampleEnum> msg) {
	//messages.push(msg);
	auto [error, n] = co_await asio::async_write(client, asio::buffer(&msg.header, sizeof(MessageHeader<ExampleEnum>)), asio::experimental::as_tuple(asio::use_awaitable));
	if (error) {
		std::cerr << error.message();
	}
	else {
		if (msg.header.bodySize > 0) {
			co_await WriteBody(std::move(msg));
		}
		else {
			//messages.pop();
		}
	}
}
asio::awaitable<void> ClientSession::WriteBody(Message<ExampleEnum> msg) {
	auto [error, n] = co_await asio::async_write(client, asio::buffer(&msg.body, msg.header.bodySize), asio::experimental::as_tuple(asio::use_awaitable));
	if (error) {

	}
	else {
		std::cout << "Sent " << n << " bytes: " << msg << "\n";
	}
	//messages.pop();
}