#pragma once

#include <memory>
#include <string>
#include "ClientSession.hpp"
#include "Message.hpp"

template<class T> class ClientSession;

template<class T> class OwnedMessage {
public:
	std::shared_ptr<ClientSession<T>> session; // should this be a shared_ptr???
	Message<T> message;
};