#pragma once

#include <asio.hpp>
#include <asio/experimental/as_tuple.hpp>
#include <thread>
#include <set>
#include <iostream>
#include "ClientSession.hpp"
#include "MutexQueue.hpp"
#include "Message.hpp"
#include "OwnedMessage.hpp"
#include "ExampleStruct.hpp"
#include "ExampleEnum.hpp"

// TODO: on client disconnect x2
// TODO: check all shared_ptrs
// TODO: thread sleep
// TODO: replace set with map

template<class T> class ClientSession;
template<class T> class OwnedMessage;

template<class T> class Server {
public:
	asio::io_context ctx;
private:
	std::jthread thr;

	asio::ip::tcp::endpoint listeningEndpoint;
	asio::ip::tcp::acceptor acceptor;
	std::set<std::shared_ptr<ClientSession<T>>> clients;
	MutexQueue<OwnedMessage<T>> incomingMessages;
	asio::awaitable<void> Listen();
	bool ValidateConnnection(std::shared_ptr<ClientSession<T>> session);
public:
	Server(const char* address, asio::ip::port_type port);
	~Server();
	void Process(); // move to protected
	void RegisterMessage(const Message<T>& msg, std::shared_ptr<ClientSession<T>> session);
protected:
	void MessageAllClients(const Message<T>& msg);
};

template<class T> Server<T>::Server(const char* address, asio::ip::port_type port) :
	listeningEndpoint(asio::ip::address::from_string(address), port),
	acceptor(ctx, listeningEndpoint),
	thr([&]() { asio::co_spawn(ctx, Listen(), asio::detached); ctx.run(); })
{
	
}
template<class T> Server<T>::~Server() {
	acceptor.close();
	ctx.stop();
	std::cout << "Server stopped\n";
}
template<class T> asio::awaitable<void> Server<T>::Listen() {
	std::cout << "Server started\n";
	while (true) {
		auto [error, client] = co_await acceptor.async_accept(asio::experimental::as_tuple(asio::use_awaitable));
		if (error) {
			std::cerr << error.message() << "\n";
		}
		else {
			// add code here to validate client connection
			std::cout << client.remote_endpoint() << " connected\n";

			auto newconn = std::make_shared<ClientSession<T>>(std::move(client), this);
			if (ValidateConnnection(newconn)) {
				co_spawn(ctx, newconn->ReadHeader(), asio::detached); // should change this
				clients.insert(std::move(newconn)); // might need to reverse the order
			}
		}
	}
}
template<class T> void Server<T>::Process() { // change this
	incomingMessages.wait();
	while (!incomingMessages.empty()) {
		auto msg = incomingMessages.pop();
		// OnMessage(msg); // maybe split here
		ExampleStruct s;
		msg.message >> s;
		std::cout << msg.session->GetClientRemoteEndpoint() << ": " << s.a << " " << s.b << "\n";
		ExampleStruct s2{ 10002, -657 };
		Message<T> m;
		m << s2;
		MessageAllClients(m);
	}
}
template<class T> bool Server<T>::ValidateConnnection(std::shared_ptr<ClientSession<T>> session) {
	return true;
}
template<class T> void Server<T>::RegisterMessage(const Message<T>& msg, std::shared_ptr<ClientSession<T>> session) {
	incomingMessages.push({ session, msg });
}
template<class T> void Server<T>::MessageAllClients(const Message<T>& msg) {
	std::set<std::shared_ptr<ClientSession<T>>> offline;
	for (auto& conn : clients) {
		if (!conn || !conn->IsConnected()) {
			offline.insert(conn);
			continue;
		}
		std::cout << "Pushed message\n";
		conn->PushMessage2(msg);
	}
	for (auto& conn : offline) {
		std::cout << conn->GetClientRemoteEndpoint() << " disconnected\n";
		clients.erase(conn);
	}
}