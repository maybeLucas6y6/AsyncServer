#pragma once

#include "Utilities.hpp"
#include "ClientSession.hpp"

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