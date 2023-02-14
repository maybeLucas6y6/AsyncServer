#pragma once

#include "Utilities.hpp"
#include "OwnedMessage.hpp"
#include "MutexQueue.hpp"

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
	asio::ip::tcp::socket client;
	MutexQueue<OwnedMessage>* messages; // should be improved 
	ClientSession(asio::ip::tcp::socket skt, MutexQueue<OwnedMessage>* msg) :
		client(std::move(skt)),
		messages(msg) 
	{
		
	}
	asio::awaitable<void> start() {
		while (true) {
			char data[5]; // change data container/type
			auto [errorReading, bytesRead] = co_await async_read(client, asio::buffer(data, 5), use_nothrow_awaitable);
			if (errorReading) {
				break;
			}
			else {
				messages->push({ shared_from_this(), data });
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