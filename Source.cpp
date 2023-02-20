#include "Server.hpp"

int main() {
	Server srv("127.0.0.1", 3000);

	srv.Process();
}