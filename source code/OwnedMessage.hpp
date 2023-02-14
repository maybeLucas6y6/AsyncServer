#pragma once

#include "Utilities.hpp"
#include "ClientSession.hpp"

class ClientSession;

class OwnedMessage {
public:
	std::shared_ptr<ClientSession> session; // should this be a shared_ptr???
	std::string_view message;
	OwnedMessage(std::shared_ptr<ClientSession> sess, std::string_view msg) : 
		session(sess), 
		message(msg) 
	{

	}
};