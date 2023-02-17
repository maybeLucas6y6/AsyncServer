#pragma once

#include <queue>
#include <mutex>
#include "OwnedMessage.hpp"

class MutexQueue {
private:
	std::queue<OwnedMessage> data;
	std::mutex mux;
	std::condition_variable condition;
	std::mutex block;
public:
	MutexQueue() = default;
	MutexQueue(const MutexQueue&) = delete;
	const OwnedMessage& front() {
		std::scoped_lock lock(mux);
		return data.front();
	}
	OwnedMessage pop() {
		std::scoped_lock lock(mux);
		auto t = std::move(data.back());
		data.pop();
		return t;
	}
	void push(const OwnedMessage& item) {
		std::scoped_lock lock(mux);
		data.push(item); // was with std::move

		std::unique_lock<std::mutex> ul(block);
		condition.notify_one();
	}
	bool empty() {
		std::scoped_lock lock(mux);
		return data.empty();
	}
	size_t count() {
		std::scoped_lock lock(mux);
		return data.size();
	}
	// clear not implemented
	void wait() {
		while (empty())
		{
			std::unique_lock<std::mutex> ul(block);
			condition.wait(ul);
		}
	}
};