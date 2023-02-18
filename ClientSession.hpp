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
	ClientSession(asio::ip::tcp::socket skt, Server* srv);
	//asio::awaitable<void> Start();
	asio::awaitable<void> Write(std::string msg);
	asio::awaitable<void> ReadHeader();
	asio::awaitable<void> ReadBody(Message<ExampleEnum> msg);
};