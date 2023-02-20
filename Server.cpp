#include "Server.hpp"

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

			auto newconn = std::make_shared<ClientSession>(std::move(client), this);
			co_spawn(listeningContext, newconn->ReadHeader(), asio::detached); // should change this
			clients.insert(std::move(newconn));
		}
	}
}
template<class T> void Server<T>::Process() { // change this
	while (true) {
		Sleep(5);
		std::cout << clients.size() << "\n";
		Message<ExampleEnum> m;
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
			Message<ExampleEnum> m;
			ExampleStruct ex{ -123,1 };
			m << ex;
			MessageAllClients(m);
		}
	}
}
template<class T> void Server<T>::RegisterMessage(const Message<ExampleEnum>& msg, std::shared_ptr<ClientSession> session) {
	messagesReceived.push({ session, msg });
}
template<class T> void Server<T>::MessageClient(const Message<ExampleEnum>& msg, std::shared_ptr<ClientSession> session) {
	if (!session || !session->IsConnected()) {
		//clients.erase(session);
	}
	else {
		session->PushMessage(msg);
	}
}
template<class T> void Server<T>::MessageAllClients(const Message<ExampleEnum>& msg) {
	std::set<std::shared_ptr<ClientSession>> offline;
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
template<class T> void Server<T>::MessageAllClientsExcept(const Message<ExampleEnum>& msg, std::shared_ptr<ClientSession> except) {
	std::set<std::shared_ptr<ClientSession>> offline;
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