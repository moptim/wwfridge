#include <iostream>
#include <variant>

#include "FridgeDB.h"
#include "WebSocketIF.h"
#include "App.h"
#include "libusockets.h"

int main()
{
	FridgeDB db("nyyla.db");
	WebSocketIF wwfridge(db);
	std::cout << "Hello world! " << (wwfridge.is_ok() ? "OK" : "Not OK") << std::endl;
	wwfridge.WaitThreads();
	return 0;
}
