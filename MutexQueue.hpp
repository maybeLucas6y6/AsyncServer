#pragma once

#include <queue>
#include <mutex>
#include <utility>

template <typename T>
class MutexQueue {
private:
	std::queue<T> data;
	std::mutex mux;
	std::condition_variable condition;
	std::mutex block;
public:
	MutexQueue() = default;
	MutexQueue(const MutexQueue<T>&) = delete;
	const T& front()
	{
		std::scoped_lock lock(mux);
		return data.front();
	}
	T pop()
	{
		std::scoped_lock lock(mux);
		auto t = std::move(data.front());
		data.pop();
		return t;
	}
	void push(const T& item)
	{
		std::scoped_lock lock(mux);
		data.emplace(std::move(item));

		std::unique_lock<std::mutex> ul(block);
		condition.notify_one();
	}
	bool empty()
	{
		std::scoped_lock lock(mux);
		return data.empty();
	}
	size_t count()
	{
		std::scoped_lock lock(mux);
		return data.size();
	}
	// clear not implemented
	void wait()
	{
		while (empty())
		{
			std::unique_lock<std::mutex> ul(block);
			condition.wait(ul);
		}
	}
};