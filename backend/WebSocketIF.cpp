#include "FridgeDB.h"
#include "WebSocketIF.h"
#include <sqlite3.h>

WebSocketIF::WebSocketIF(FridgeDB &fridgeDB, int numWSWorkerThreads, int port)
	: m_port(port)
	, m_ok(true)
{
	int rv, i;

	if (numWSWorkerThreads <= 0)
		numWSWorkerThreads = std::thread::hardware_concurrency();

	for (i = 0; i < numWSWorkerThreads; i++) {
		std::optional<FridgeDB::Connection> conn = fridgeDB.GetNewConnection();

		if (conn.has_value()) {
			m_wsWorkerPool.push_back(
				std::thread(WSWorkerMainloop, std::move(*conn), m_port)
			);
		}
	}
	m_ok = m_wsWorkerPool.size() != 0;
}

void WebSocketIF::WaitThreads(void)
{
	for (auto &wsWorker : m_wsWorkerPool)
		wsWorker.join();
}

uWS::App WebSocketIF::CreateWSWorker(FridgeDB::Connection dbConn)
{
	struct PerSocketData {};

	return uWS::App().ws<PerSocketData>("/query", {
		.message = [dbConn = std::move(dbConn)]
			(auto *ws, std::string_view msgView, uWS::OpCode opCode) mutable
		{
			const std::string msg(msgView);
			ws->send(dbConn.Query(msg), opCode, true);
		},
	});
}

void WebSocketIF::WSWorkerMainloop(FridgeDB::Connection dbConn, int port)
{
	CreateWSWorker(std::move(dbConn)).listen(port, [port](auto *listen_socket) {
		if (listen_socket != nullptr) {
			std::cout << "Listening on " << port << std::endl;
		}
	}).run();
}
