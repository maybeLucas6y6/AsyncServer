#include "ExampleEnum.hpp"
#include "Message.hpp"
#include "ClientSession.hpp"
#include "Server.hpp"
#include <asio/experimental/as_tuple.hpp>
#include <iostream>
//#include <cstdint>
//enum class ExampleEnum : uint32_t;

ClientSession::ClientSession(asio::ip::tcp::socket skt, Server* srv) :
	client(std::move(skt)),
	server(srv)
{
	msg.body.resize(32);
}
asio::awaitable<void> ClientSession::Write(std::string msg) {
	auto [errorWriting, bytesWritten] = co_await async_write(client, asio::buffer(msg, sizeof(msg)), asio::experimental::as_tuple(asio::use_awaitable));
	if (errorWriting) {
		//std::cerr << errorWriting.message();
		std::cout << client.remote_endpoint() << " disconnected\n";
		client.close();
	}
}
asio::awaitable<void> ClientSession::ReadHeader() {
	while (true) {
		auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(&msg.header, sizeof(MessageHeader<ExampleEnum>)), asio::experimental::as_tuple(asio::use_awaitable));
		if (errorReading) {
			//std::cerr << errorReading.message();
			std::cout << client.remote_endpoint() << " disconnected\n";
			client.close();
		}
		else {
			if (msg.header.bodySize > 0) {
				msg.body.resize(msg.header.bodySize); // this should be changed
				co_await ReadBody();
			}
			else {
				std::cout << "Received " << bytesRead << " bytes: " << msg << "\n";
				server->RegisterMessage(shared_from_this(), msg);
			}
		}
	}
}
asio::awaitable<void> ClientSession::ReadBody() {
	auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(&msg.body, msg.header.bodySize), asio::experimental::as_tuple(asio::use_awaitable));
	if (errorReading) {
		//std::cerr << errorReading.message();
		std::cout << client.remote_endpoint() << " disconnected\n";
		client.close();
	}
	else {
		std::cout << "Received " << bytesRead << " bytes: " << msg << "\n";
		//server->RegisterMessage(shared_from_this(), msg);
	}
}
//asio::awaitable<void> ClientSession::WriteHeader(Message<ExampleEnum> msg) {
//	auto [error, n] = co_await asio::async_write(socket, asio::buffer(&msg.header, sizeof(MessageHeader<ExampleEnum>)), asio::experimental::as_tuple(asio::use_awaitable));
//	if (error) {
//		
//	}
//	else {
//		if (msg.header.bodySize > 0) {
//			WriteBody(std::move(msg));
//		}
//	}
//}
//asio::awaitable<void> ClientSession::WriteBody(Message<ExampleEnum> msg) {
//	auto [error, n] = co_await asio::async_write(socket, asio::buffer(&msg.body, msg.BodySize()), asio::experimental::as_tuple(asio::use_awaitable));
//	if (error) {
//
//	}
//	else {
//		
//	}
//}