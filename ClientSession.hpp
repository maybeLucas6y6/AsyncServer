#pragma once

#include <asio.hpp>
#include <memory>
#include "Message.hpp"
#include "MutexQueue.hpp"
#include <iostream>
#include <asio/experimental/as_tuple.hpp>
#include "Server.hpp"

template<class T> class Server;

template<class T> class ClientSession : public std::enable_shared_from_this<ClientSession<T>> {
private:
	bool isConnected;
	asio::ip::tcp::socket client;
	Server<T>* server;
	Message<T> message;
	MutexQueue<Message<T>> messagesPushed;
public:
	ClientSession(asio::ip::tcp::socket skt, Server<T>* srv);
	~ClientSession();
	void PushMessage(const Message<T>& msg);
	void PushMessage2(const Message<T>& msg);
	bool IsConnected() const;
	asio::ip::tcp::endpoint GetClientRemoteEndpoint() const;
	asio::awaitable<void> ReadHeader();
private:
	asio::awaitable<void> ReadBody();
	asio::awaitable<void> WriteHeader();
	asio::awaitable<void> WriteBody();
};

template<class T> ClientSession<T>::ClientSession(asio::ip::tcp::socket skt, Server<T>* srv) :
	client(std::move(skt)),
	server(srv)
{
	isConnected = true;
	std::cout << "Session created\n";
}
template<class T> ClientSession<T>::~ClientSession() {
	client.cancel();
	client.close();
	std::cout << "Session terminated\n";
}
template<class T> void ClientSession<T>::PushMessage(const Message<T>& msg) {
	messagesPushed.push(msg);
}
template<class T> bool ClientSession<T>::IsConnected() const {
	return isConnected;
}
template<class T> asio::ip::tcp::endpoint ClientSession<T>::GetClientRemoteEndpoint() const {
	return client.remote_endpoint();
}
template<class T> asio::awaitable<void> ClientSession<T>::ReadHeader() {
	while (isConnected) {
		std::cout << "Started reading header\n";
		auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(&message.header, sizeof(MessageHeader<T>)), asio::experimental::as_tuple(asio::use_awaitable));
		std::cout << "Read header of size " << bytesRead << "\n";
		if (errorReading) {
			std::cerr << errorReading.message() << "\n";
			isConnected = false;
			break;
		}
		if (message.header.bodySize > 0) {
			message.body.resize(message.header.bodySize);
			co_await ReadBody();
		}
		else {
			server->RegisterMessage(message, this->shared_from_this());
		}
	}
}
template<class T> asio::awaitable<void> ClientSession<T>::ReadBody() {
	std::cout << "Started reading body\n";
	auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(message.body.data(), message.header.bodySize), asio::experimental::as_tuple(asio::use_awaitable));
	std::cout << "Read body of size " << bytesRead << "\n";
	if (errorReading) {
		std::cerr << errorReading.message() << "\n";
		isConnected = false;
	}
	else {
		server->RegisterMessage(message, this->shared_from_this());
	}
}
template<class T> void ClientSession<T>::PushMessage2(const Message<T>& msg) {
	bool free = messagesPushed.empty();
	messagesPushed.push(msg);
	if (free) {
		std::cout << "Started writing in pm2\n";
		asio::co_spawn(server->ctx, WriteHeader(), asio::detached);
	}
}
template<class T> asio::awaitable<void> ClientSession<T>::WriteHeader() {
	while (!messagesPushed.empty() && isConnected) {
		std::cout << "Messages in pqueue at start: " << messagesPushed.count() << "\n";
		auto [error, n] = co_await asio::async_write(client, asio::buffer(&messagesPushed.front().header, sizeof(MessageHeader<T>)), asio::experimental::as_tuple(asio::use_awaitable));
		std::cout << "Written header: " << n << " bytes\n";
		if (error) {
			std::cerr << error.message() << "\n";
			isConnected = false;
			break;
		}
		if (messagesPushed.front().header.bodySize > 0) {
			co_await WriteBody();
		}
		else {
			messagesPushed.pop();
		}
	}
}
template<class T> asio::awaitable<void> ClientSession<T>::WriteBody() {
	std::cout << "Header says: " << messagesPushed.front().header.bodySize << "\n";
	auto [error, n] = co_await asio::async_write(client, asio::buffer(messagesPushed.front().body.data(), messagesPushed.front().header.bodySize), asio::experimental::as_tuple(asio::use_awaitable));
	std::cout << "Written body: " << n << " bytes\n";
	if (error) {
		std::cerr << error.message() << "\n";
		isConnected = false;
	}
	messagesPushed.pop();
}