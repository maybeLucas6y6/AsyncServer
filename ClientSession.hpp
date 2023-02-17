#pragma once

#include <memory>
#include <asio.hpp>

class Server;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
	asio::ip::tcp::socket client;
	Server* server;
	ClientSession(asio::ip::tcp::socket skt, Server* srv);
	asio::awaitable<void> Start();
	asio::awaitable<void> Write(std::string msg);
};