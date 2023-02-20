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

// TODO: on client disconnect x2
// TODO: check all shared_ptrs
// TODO: thread sleep
// TODO: replace set with map

template<class T> class ClientSession;
template<class T> class OwnedMessage;

template<class T> class Server {
public:
	asio::io_context processingContext;
private:
	std::jthread listeningThread;
	asio::io_context listeningContext;
	std::jthread processingThread;
	asio::executor_work_guard<decltype(processingContext.get_executor())> work;
	asio::ip::tcp::endpoint listeningEndpoint;
	asio::ip::tcp::acceptor acceptor;
	std::set<std::shared_ptr<ClientSession<T>>> clients;
	MutexQueue<OwnedMessage<T>> messagesReceived;
	asio::awaitable<void> Listen();
public:
	Server(const char* address, asio::ip::port_type port);
	~Server();
	void Process(); // move to protected
	void RegisterMessage(const Message<T>& msg, std::shared_ptr<ClientSession<T>> session);
protected:
	void MessageClient(const Message<T>& msg, std::shared_ptr<ClientSession<T>> client);
	void MessageAllClients(const Message<T>& msg);
	void MessageAllClientsExcept(const Message<T>& msg, std::shared_ptr<ClientSession<T>> except);
};

template<class T> Server<T>::Server(const char* address, asio::ip::port_type port) :
	listeningEndpoint(asio::ip::address::from_string(address), port),
	acceptor(listeningContext, listeningEndpoint),
	listeningThread([&] { listeningContext.run(); }),
	work(processingContext.get_executor()),
	processingThread([&] { processingContext.run(); })
{
	asio::co_spawn(listeningContext, Listen(), asio::detached);
	std::cout << "Server started\n";
}
template<class T> Server<T>::~Server() {
	listeningContext.stop();
	processingContext.stop();
	acceptor.close();
	std::cout << "Server stopped\n";
}
template<class T> asio::awaitable<void> Server<T>::Listen() {
	while (true) {
		auto [error, client] = co_await acceptor.async_accept(asio::experimental::as_tuple(asio::use_awaitable));
		if (error) {
			std::cerr << error.message();
		}
		else {
			// add code here to validate client connection
			std::cout << client.remote_endpoint() << " connected\n";

			auto newconn = std::make_shared<ClientSession<T>>(std::move(client), this);
			co_spawn(listeningContext, newconn->ReadHeader(), asio::detached); // should change this
			clients.insert(std::move(newconn));
		}
	}
}
template<class T> void Server<T>::Process() { // change this
	while (true) {
		Message<T> m;
		ExampleStruct ex{ -123,1 };
		m << ex;
		MessageAllClients(m);
	}
	while (true) {
		messagesReceived.wait();
		while (!messagesReceived.empty()) {
			auto msg = messagesReceived.pop();
			ExampleStruct s;
			msg.message >> s;
			std::cout << msg.session->GetClientRemoteEndpoint() << ": " << s.a << " " << s.b << "\n";
			Message<T> m;
			ExampleStruct ex{ -123,1 };
			m << ex;
			MessageAllClients(m);
		}
	}
}
template<class T> void Server<T>::RegisterMessage(const Message<T>& msg, std::shared_ptr<ClientSession<T>> session) {
	messagesReceived.push({ session, msg });
}
template<class T> void Server<T>::MessageClient(const Message<T>& msg, std::shared_ptr<ClientSession<T>> client) {
	if (!client || !client->IsConnected()) {
		std::cout << client->GetClientRemoteEndpoint() << " disconnected\n";
		//clients.erase(session);
	}
	else {
		client->PushMessage(msg);
	}
}
template<class T> void Server<T>::MessageAllClients(const Message<T>& msg) {
	std::set<std::shared_ptr<ClientSession<T>>> offline;
	for (auto& conn : clients) {
		if (!conn || !conn->IsConnected()) {
			offline.insert(conn);
			continue;
		}
		conn->PushMessage(msg);
	}
	for (auto& conn : offline) {
		std::cout << conn->GetClientRemoteEndpoint() << " disconnected\n";
		//clients.erase(conn);
	}
}
template<class T> void Server<T>::MessageAllClientsExcept(const Message<T>& msg, std::shared_ptr<ClientSession<T>> except) {
	std::set<std::shared_ptr<ClientSession<T>>> offline;
	for (auto& conn : clients) {
		if (!conn || !conn->IsConnected()) {
			offline.insert(conn);
			continue;
		}
		if (conn == except) {
			continue;
		}
		conn->PushMessage(msg);
	}
	for (auto& conn : offline) {
		std::cout << conn->GetClientRemoteEndpoint() << " disconnected\n";
		//clients.erase(conn);
	}
}