#include "Server.hpp"
#include "ExampleEnum.hpp"

int main() {
	Server<ExampleEnum> srv("127.0.0.1", 3000);

	srv.Process();
}