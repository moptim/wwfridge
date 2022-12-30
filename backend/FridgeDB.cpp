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
	sqlite3_stmt *raw_stmt;

	int rv = sqlite3_prepare_v2(m_conn,
	                            statementString.c_str(),
	                            statementString.size(),
	                            &raw_stmt,
	                            nullptr);

	if (rv != SQLITE_OK) {
		sqlite3_finalize(raw_stmt);
		raw_stmt = nullptr;
	}
	return raw_stmt;
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

		sqlite3_stmt *raw_stmt = CompileSingleStatement(statementString);
		if (raw_stmt != nullptr)
			m_statements.push_back({
				.command = i.command,
				.BindParams = i.BindParams,
				.RowCallback = i.RowCallback,
				.raw_stmt = raw_stmt,
			});
		else
			std::cerr << "Failed to compile statement " <<
				command << ": " << sqlite3_errmsg(m_conn) <<
				std::endl;
	}
}

void FridgeDB::Connection::MaybeInitializeDB()
{
	for (auto &raw_stmt : m_initializers) {
		int rv;

		for (;;) {
			rv = sqlite3_step(raw_stmt);
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
		sqlite3_reset(raw_stmt);
	}
}

void FridgeDB::Connection::DestroyStatements()
{
	for (auto &initializer : m_initializers) {
		sqlite3_finalize(initializer);
	}
	for (auto &i : m_statements) {
		sqlite3_finalize(i.raw_stmt);
	}
}

std::string FridgeDB::Connection::Query(const std::string &request)
{
	JSONStaticReplies &replies = JSONStaticReplies::GetInstance();

	nlohmann::json req = nlohmann::json::parse(request, nullptr, false, true);
	if (req.is_discarded())
		return replies.GetNotJSONError();

	return PerformDBRequest(req);
}

std::string FridgeDB::Connection::PerformDBRequest(const nlohmann::json &request)
{
	JSONStaticReplies &replies = JSONStaticReplies::GetInstance();
	sqlite3_stmt *raw_stmt;

	const auto &requestNameIt = request.find("request");
	if (requestNameIt == request.cend())
		return replies.GetNoSuchRequestError();

	const std::string &requestName = *requestNameIt;

	auto statement = std::find_if(m_statements.begin(), m_statements.end(),
		[requestName](const auto &statement) {
			return statement.command == requestName;
		}
	);
	if (statement == m_statements.end())
		return replies.GetNoSuchRequestError();

	raw_stmt = statement->raw_stmt;
	statement->BindParams(raw_stmt);

	nlohmann::json reply({
		{"success", "false"},
	});
	for (;;) {
		int rv = sqlite3_step(raw_stmt);
		switch (rv) {
		case SQLITE_BUSY:
			continue;
		case SQLITE_MISUSE:
			std::cerr << "SQLITE_MISUSE on " << requestName <<
				std::endl;
			goto out_reset;
		case SQLITE_DONE:
			goto out_success;
		case SQLITE_ROW:
			nlohmann::json jsonizedRow = statement->RowCallback(raw_stmt);
			if (reply.find("values") == reply.cend())
				reply["values"] = nlohmann::json::array();

			reply["values"].push_back(jsonizedRow);
			break;
		};
	}
	out_success:
	reply["success"] = true;

	out_reset:
	sqlite3_reset(raw_stmt);
	return nlohmann::to_string(reply);
}

FridgeDB::Connection::~Connection()
{
	if (m_conn != nullptr)
		sqlite3_close(m_conn);
}

FridgeDB::JSONStaticReplies::JSONStaticReplies()
{
	nlohmann::json notJSONError = {{"message", "error: not JSON"}};
	nlohmann::json noSuchRequestError = {{"message", "error: no such request"}};

	m_notJSONError = nlohmann::to_string(notJSONError);
	m_noSuchRequestError = nlohmann::to_string(noSuchRequestError);
}

FridgeDB::JSONStaticReplies &FridgeDB::JSONStaticReplies::GetInstance()
{
	static JSONStaticReplies instance;
	return instance;
}
