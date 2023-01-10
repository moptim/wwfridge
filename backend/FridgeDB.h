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

	class Statement {
	public:
		Statement() : m_rawStmt(nullptr) {}
		Statement(sqlite3 *conn, std::string_view stmtStr);

		Statement(Statement &&);
		Statement &operator=(Statement &&);
		Statement(const Statement &) = delete;
		Statement &operator=(const Statement &) = delete;

		bool Ok() const { return m_rawStmt != nullptr; }
		sqlite3_stmt *Get() { return m_rawStmt; }

		~Statement();

	private:
		sqlite3_stmt *m_rawStmt;
	};

	class StatementIterator;

	Connection(Connection &&) = default;
	Connection &operator=(Connection &&) = default;
	Connection(const Connection &) = delete;
	Connection &operator=(const Connection &) = delete;

	std::string Query(const std::string &request);

	~Connection();

protected:
	Connection(sqlite3 *raw_conn);

private:
	bool CompileInitializers();
	bool CompileQueryStatements();
	void MaybeInitializeDB();

	std::string PerformDBRequest(const nlohmann::json &request);

	nlohmann::json GetItemsInFridge();
	nlohmann::json AddItemsToFridge(const nlohmann::json::array_t &items);

	sqlite3 *m_conn;

	std::vector<Statement> m_initializers;

	Statement m_beginTransaction;
	Statement m_endTransaction;
	Statement m_getItemsInFridge;
	Statement m_addItemsToFridgeInsertClass;
	Statement m_addItemsToFridgeUpdateClass;
	Statement m_addItemsToFridgeInsertItem;

	static const std::string k_getItemsInFridgeQuery;
	static const std::string k_addItemsToFridgeQuery;

	static const std::string k_requestKey;
	static const std::string k_itemsKey;
	static const std::string k_successKey;
	static const std::string k_valuesKey;
	static const nlohmann::json k_emptyResponseJSON;
};

class FridgeDB::Connection::StatementIterator {
public:
	friend class FridgeDB::Connection;

	StatementIterator(StatementIterator &&) = default;
	StatementIterator &operator=(StatementIterator &&) = default;
	StatementIterator(const StatementIterator &) = delete;
	StatementIterator &operator=(const StatementIterator &) = delete;

	StatementIterator &operator++();
	sqlite3_stmt *operator*() { return m_rawStmt; }
	bool Ok() const { return m_ok; }
	bool Done() const { return m_done; }

	~StatementIterator();
protected:
	StatementIterator(sqlite3_stmt *rawStmt, const std::string &requestName = "<unnamed request>");

private:
	sqlite3_stmt *m_rawStmt;
	const std::string &m_requestName;
	bool m_ok;
	bool m_done;
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
	const std::string &GetItemsNotDefinedError() const {
		return m_itemsNotDefinedError;
	}

private:
	JSONStaticReplies();

	std::string m_notJSONError;
	std::string m_noSuchRequestError;
	std::string m_itemsNotDefinedError;
};

#endif
