#include "Server.hpp"

int main() {
	Server srv("127.0.0.1", 3000);

	// suspend main thread execution
	srv.Process();
	/*auto mainThread = OpenThread(THREAD_ALL_ACCESS, FALSE, GetCurrentThreadId());
	SuspendThread(mainThread);*/
}