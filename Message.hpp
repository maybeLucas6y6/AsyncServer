#pragma once

#include <cstdint>
#include <vector>
#include <iostream>

template<class T>
class MessageHeader {
public:
	T id{};
	uint32_t bodySize;
};

template<class T>
class Message {
public:
	MessageHeader<T> header;
	std::vector<uint8_t> body;
	size_t BodySize() const {
		return body.size();
	}
	friend std::ostream& operator << (std::ostream& os, const Message<T>& msg)
	{
		os << "ID:" << int(msg.header.id) << " Size:" << msg.header.bodySize;
		return os;
	}
	template<typename DataType>	friend Message<T>& operator << (Message<T>& msg, const DataType& data)
	{
		static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pushed into vector");

		size_t i = msg.body.size();
		msg.body.resize(msg.body.size() + sizeof(DataType));
		std::memcpy(msg.body.data() + i, &data, sizeof(DataType));
		msg.header.bodySize = msg.BodySize();

		return msg;
	}
	template<typename DataType>	friend Message<T>& operator >> (Message<T>& msg, DataType& data)
	{
		static_assert(std::is_standard_layout<DataType>::value, "Data is too complex to be pulled from vector");

		size_t i = msg.body.size() - sizeof(DataType);
		std::memcpy(&data, msg.body.data() + i, sizeof(DataType));
		msg.body.resize(i);
		msg.header.bodySize = msg.BodySize();

		return msg;
	}
};