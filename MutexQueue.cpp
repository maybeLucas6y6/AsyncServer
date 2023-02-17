#include "MutexQueue.hpp"

template<typename T>
const T& MutexQueue<T>::front()
{
	std::scoped_lock lock(mux);
	return data.front();
}
template<typename T>
T MutexQueue<T>::pop()
{
	std::scoped_lock lock(mux);
	auto t = std::move(data.back());
	data.pop();
	return t;
}
template<typename T>
void MutexQueue<T>::push(const T& item)
{
	std::scoped_lock lock(mux);
	data.push(std::move(item));

	std::unique_lock<std::mutex> ul(block);
	condition.notify_one();
}
template<typename T>
bool MutexQueue<T>::empty()
{
	std::scoped_lock lock(mux);
	return data.empty();
}
template<typename T>
size_t MutexQueue<T>::count()
{
	std::scoped_lock lock(mux);
	return data.size();
}
template<typename T>
void MutexQueue<T>::wait()
{
	while (empty())
	{
		std::unique_lock<std::mutex> ul(block);
		condition.wait(ul);
	}
}