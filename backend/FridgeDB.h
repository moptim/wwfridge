#ifndef FRIDGEDB_H_
#define FRIDGEDB_H_

#include "FridgeDBHandlers.h"
#include <optional>
#include <string>
#include <vector>
#include <sqlite3.h>

class FridgeDB {
public:
	class Connection;
	class JSONStaticReplies;

	FridgeDB(std::string filename);

	std::optional<Connection> GetNewConnection();

private:
	std::string m_filename;
};

class FridgeDB::Connection {
public:
	friend class FridgeDB;

	Connection(Connection &&) = default;
	Connection &operator=(Connection &&) = default;
	Connection(const Connection &) = delete;
	Connection &operator=(const Connection &) = delete;

	std::string Query(const std::string &request);

	~Connection();

protected:
	Connection(sqlite3 *raw_conn);

private:
	sqlite3_stmt *CompileSingleStatement(const std::string &statementString);
	void CompileStatements();
	void MaybeInitializeDB();

	std::string PerformDBRequest(const nlohmann::json &request);

	void DestroyStatements();

	sqlite3 *m_conn;
	std::vector<sqlite3_stmt *> m_initializers;
	// std::vector<std::pair<std::string, sqlite3_stmt *>> m_statements;

	std::vector<FridgeDBHandlers::FridgeDBHandler> m_statements;
};

class FridgeDB::JSONStaticReplies {
public:
	friend class FridgeDB::Connection;

protected:
	JSONStaticReplies(const JSONStaticReplies &)	= delete;
	void operator=(const JSONStaticReplies &)	= delete;

	static JSONStaticReplies &GetInstance();

	const std::string &GetNotJSONError() const {
		return m_notJSONError;
	}
	const std::string &GetNoSuchRequestError() const {
		return m_noSuchRequestError;
	}

private:
	JSONStaticReplies();

	std::string m_notJSONError;
	std::string m_noSuchRequestError;
};

#endif
