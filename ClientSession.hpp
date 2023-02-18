#pragma once

#include <memory>
#include <asio.hpp>
#include "Message.hpp"
//#include "ExampleEnum.hpp"
#include <cstdint>
enum class ExampleEnum : uint32_t;

class Server;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
	asio::ip::tcp::socket client;
	Server* server;
	Message<ExampleEnum> msg;
	ClientSession(asio::ip::tcp::socket skt, Server* srv);
	asio::awaitable<void> Write(std::string msg);
	asio::awaitable<void> ReadHeader();
	asio::awaitable<void> ReadBody();
	//asio::awaitable<void> WriteHeader(Message<ExampleEnum> msg);
	//asio::awaitable<void> WriteBody(Message<ExampleEnum> msg);
};