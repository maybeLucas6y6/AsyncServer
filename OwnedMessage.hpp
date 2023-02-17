#pragma once

#include <memory>
#include <string>
#include "ClientSession.hpp"

class ClientSession;

class OwnedMessage {
public:
	std::shared_ptr<ClientSession> session; // should this be a shared_ptr???
	std::string message;
	OwnedMessage(std::shared_ptr<ClientSession> sess, std::string msg);
};