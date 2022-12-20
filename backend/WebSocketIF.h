#ifndef WEBSOCKETIF_H_
#define WEBSOCKETIF_H_

#include "FridgeDB.h"
#include "App.h"
#include <memory>
#include <thread>
#include <vector>

class WebSocketIF {
public:
	WebSocketIF(FridgeDB &db, int numWSWorkerThreads = 0, int port = 32000);

	bool is_ok() const { return m_ok; }

	void WaitThreads();

private:
	static uWS::App CreateWSWorker(FridgeDB::Connection dbConn);
	static void WSWorkerMainloop(FridgeDB::Connection dbConn, int port);

	std::vector<std::thread> m_wsWorkerPool;

	int m_port;
	bool m_ok;
};

#endif
