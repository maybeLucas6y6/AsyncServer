#pragma once

#include <memory>
#include <asio.hpp>
#include "Message.hpp"
//#include "ExampleEnum.hpp"
#include <cstdint>
enum class ExampleEnum : uint32_t;
#include "MutexQueue.hpp"

class Server;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
	asio::ip::tcp::socket client;
	Server* server;
	Message<ExampleEnum> message;
	MutexQueue<Message<ExampleEnum>> messages;
	ClientSession(asio::ip::tcp::socket skt, Server* srv);
	~ClientSession();
	asio::awaitable<void> ReadHeader();
	asio::awaitable<void> ReadBody();
	asio::awaitable<void> WriteHeader();
	asio::awaitable<void> WriteBody();
};