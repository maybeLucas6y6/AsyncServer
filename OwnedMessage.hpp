#pragma once

#include <memory>
#include <string>
#include "ClientSession.hpp"
#include "Message.hpp"
//#include "ExampleEnum.hpp"
#include <cstdint>
enum class ExampleEnum : uint32_t;
class ClientSession;

class OwnedMessage {
public:
	std::shared_ptr<ClientSession> session; // should this be a shared_ptr???
	Message<ExampleEnum> message;
};