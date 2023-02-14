#include <iostream>
#include <thread>
#include <string>
#include <vector>
#include <queue>

#define ASIO_STANDALONE
#include <asio.hpp>
#include <asio/experimental/as_tuple.hpp>
constexpr auto use_nothrow_awaitable = asio::experimental::as_tuple(asio::use_awaitable);

int id = 0;

class ClientSession;

class OwnedMessage {
public:
	ClientSession* session;
	std::string_view message;
	int id;
	OwnedMessage(ClientSession* sess, std::string_view msg, int uid) : session(sess), message(msg) {
		id = uid;
	}
};

std::queue<OwnedMessage> messages;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
	asio::ip::tcp::socket client;
	int id; // should be improved
	ClientSession(asio::ip::tcp::socket skt, int uid) :
		client(std::move(skt)) {
		id = uid;
	}
	asio::awaitable<void> start() {
		while (true) {
			char data[5]; // change data container/type
			auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(data, 5), use_nothrow_awaitable);
			if (errorReading) {
				break;
			}
			else {
				messages.push({ this, data, id });
			}
			continue; // change this, this should be on client side I think
			auto [errorWriting, bytesWritten] = co_await async_write(client, asio::buffer(data, bytesRead), use_nothrow_awaitable);
			if (errorWriting) {
				break;
			}
		}
		client.close();
	}
};

class Server {
private:
	std::thread serverThread;
	asio::io_context serverContext;
	asio::ip::address address;
	asio::ip::port_type port;
	asio::ip::tcp::endpoint listeningEndpoint;
	asio::ip::tcp::acceptor acceptor;
	std::vector<std::shared_ptr<ClientSession>> clients; // it's shared_ptr because you can't copy/duplicate sockets
public:
	Server(const char* addr, asio::ip::port_type pt) :
		address(asio::ip::address::from_string(addr)),
		port(pt),
		listeningEndpoint(address, port),
		acceptor(serverContext, listeningEndpoint) {

	}
	~Server() {
		serverContext.stop();
		if (serverThread.joinable()) {
			serverThread.join();
		}
		std::cout << "[SERVER] Stopped!\n";
	}
	void start() {
		asio::co_spawn(serverContext, listen(), asio::detached);
		serverThread = std::thread([this] {
			serverContext.run();
			});
	}
	asio::awaitable<void> update() {
		while (true) {
			while (messages.empty()) {

			}
			// take the first message in queue and send it to all clients except the one who sent it
			for (auto& conn : clients) {
				if (conn->id == messages.front().id) { // currently uses an int id, should update to pointer id or smth better
					continue;
				}
				auto [e, n] = co_await async_write(conn->client, asio::buffer(messages.front().message), use_nothrow_awaitable);
			}
			messages.pop();
		}
	}
	asio::awaitable<void> listen() {
		while (true) {
			auto [error, client] = co_await acceptor.async_accept(use_nothrow_awaitable);
			if (error) {
				std::cerr << error.message();
			}
			else {
				// on client connect
				auto [errorWriting, bytesWritten] = co_await async_write(client, asio::buffer("Connected!\r\n"), use_nothrow_awaitable);
				if (errorWriting || bytesWritten == 0) {
					std::cerr << errorWriting.message() << ", bytes written: " << bytesWritten << "\n";
				}
				else {
					// make new session with newly connected client
					auto newconn = std::make_shared<ClientSession>(std::move(client), id++);
					clients.push_back(std::move(newconn));
					co_spawn(serverContext, clients.back()->start(), asio::detached);
				}
			}
		}
	}
};

int main() {
	Server srv("127.0.0.1", 3000);
	srv.start(); // this started a coroutines context in a different thread than main

	asio::io_context context; // this context start in main thread
	asio::co_spawn(context, srv.update(), asio::detached);
	context.run(); // coroutines here

	// should implement another thread
}