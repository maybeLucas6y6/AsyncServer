#pragma once

#include <asio.hpp>
#include <memory>
#include "ExampleEnum.hpp"
#include "Message.hpp"
#include "MutexQueue.hpp"

class Server;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
private:
	bool isConnected;
	asio::ip::tcp::socket client;
	Server* server;
	Message<ExampleEnum> message;
	MutexQueue<Message<ExampleEnum>> messages;
public:
	ClientSession(asio::ip::tcp::socket skt, Server* srv);
	~ClientSession();
	void PushMessage(const Message<ExampleEnum>& msg);
	bool IsConnected() const;
	asio::ip::tcp::endpoint GetClientRemoteEndpoint() const;
	asio::awaitable<void> ReadHeader();
private:
	asio::awaitable<void> ReadBody();
	asio::awaitable<void> WriteHeader();
	asio::awaitable<void> WriteBody();
};