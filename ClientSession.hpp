#pragma once

#include "Utilities.hpp"
#include "OwnedMessage.hpp"

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
	asio::ip::tcp::socket client;
	int id; // should be improved
	std::queue<OwnedMessage>* messages;
	ClientSession(asio::ip::tcp::socket skt, int uid, std::queue<OwnedMessage>* msg) :
		client(std::move(skt)),
		messages(msg) {
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
				messages->push({ this, data, id });
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