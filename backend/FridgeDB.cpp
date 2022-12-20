#include "FridgeDB.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include <optional>
#include <sqlite3.h>

FridgeDB::FridgeDB(std::string filename)
	: m_filename(filename)
{
}

std::optional<FridgeDB::Connection> FridgeDB::GetNewConnection()
{
	int rv;
	sqlite3 *raw_conn;
	std::optional<FridgeDB::Connection> conn;

	rv = sqlite3_open(m_filename.c_str(), &raw_conn);
	if (rv != SQLITE_OK) {
		std::cerr << "WebSocketIF: failed to open database " <<
			m_filename << ": " << sqlite3_errmsg(raw_conn) <<
			std::endl;
		sqlite3_close(raw_conn);
	} else {
		conn = Connection(raw_conn);
	}
	return conn;
}

FridgeDB::Connection::Connection(sqlite3 *raw_conn)
	: m_conn(raw_conn)
{}

std::string FridgeDB::Connection::Query(const std::string &request)
{
	return std::string("FridgeDB esponding to query: ") + request;
}

FridgeDB::Connection::~Connection()
{
	if (m_conn != nullptr)
		sqlite3_close(m_conn);
}
