#include "FridgeDB.h"
#include "FridgeDBHandlers.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include <optional>
#include <sqlite3.h>

constexpr const int k_connTimeoutMs = 10;

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
		std::cerr << "FridgeDB: failed to open database " <<
			m_filename << ": " << sqlite3_errmsg(raw_conn) <<
			std::endl;
		sqlite3_close(raw_conn);
	} else {
		sqlite3_busy_timeout(raw_conn, k_connTimeoutMs);
		conn = Connection(raw_conn);
	}
	return conn;
}

FridgeDB::Connection::Connection(sqlite3 *raw_conn)
	: m_conn(raw_conn)
{
	CompileStatements();
	MaybeInitializeDB();
}

sqlite3_stmt *FridgeDB::Connection::CompileSingleStatement(
	const std::string &statementString)
{
	sqlite3_stmt *statement;

	int rv = sqlite3_prepare_v2(m_conn,
	                            statementString.c_str(),
	                            statementString.size(),
	                            &statement,
	                            nullptr);

	if (rv != SQLITE_OK) {
		sqlite3_finalize(statement);
		statement = nullptr;
	}
	return statement;
}

void FridgeDB::Connection::CompileStatements()
{
	for (const auto &statementView : FridgeDBHandlers::initializers) {
		const std::string statementString(statementView);

		sqlite3_stmt *statement = CompileSingleStatement(statementString);
		if (statement != nullptr) {
			m_initializers.push_back(statement);
		} else {
			std::cerr << "Failed to compile an initializer statement: " <<
				sqlite3_errmsg(m_conn) << std::endl <<
				statementString << std::endl;
		}
		m_initializers.push_back(statement);
	}

	for (const auto &i : FridgeDBHandlers::handlers) {
		const std::string command(i.command);
		const std::string statementString(i.statementString);

		sqlite3_stmt *statement = CompileSingleStatement(statementString);
		if (statement != nullptr)
			m_statements.push_back(std::make_pair(command, statement));
		else
			std::cerr << "Failed to compile statement " <<
				command << ": " << sqlite3_errmsg(m_conn) <<
				std::endl;
	}
}

void FridgeDB::Connection::MaybeInitializeDB()
{
	for (auto &stmt : m_initializers) {
		int rv;

		for (;;) {
			rv = sqlite3_step(stmt);
			switch (rv) {
			case SQLITE_BUSY:
				// Retry after busy handler has done its thing
				continue;
			case SQLITE_MISUSE:
				std::cerr << "SQLITE_MISUSE in MaybeInitializeDB" <<
					std::endl;
				goto out_reset;
			case SQLITE_DONE:
				goto out_reset;
			}
		}
		out_reset:
		sqlite3_reset(stmt);
	}
}

void FridgeDB::Connection::DestroyStatements()
{
	for (auto &i : m_statements) {
		sqlite3_stmt *statement = i.second;
		sqlite3_finalize(statement);
	}
}

std::string FridgeDB::Connection::Query(const std::string &request)
{
	nlohmann::json req = nlohmann::json::parse(request, nullptr, false, true);
	if (req.is_discarded())
		return std::string("not valid json lol (todo)");

	return std::string("FridgeDB esponding to query: ") + request;
}

FridgeDB::Connection::~Connection()
{
	if (m_conn != nullptr)
		sqlite3_close(m_conn);
}
