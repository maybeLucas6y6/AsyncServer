#include "Server.hpp"

int main() {
	Server srv("127.0.0.1", 3003);

	// suspend main thread execution
	auto mainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
	SuspendThread(mainThread);
}