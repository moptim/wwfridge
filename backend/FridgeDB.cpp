#include "FridgeDB.h"
#include "FridgeDBHandlers.h"
#include "nlohmann/json.hpp"
#include <iostream>
#include <optional>
#include <sqlite3.h>

constexpr const int k_connTimeoutMs = 10;

const std::string FridgeDB::Connection::k_getItemsInFridgeQuery = "GetItemsInFridge";
const std::string FridgeDB::Connection::k_addItemsToFridgeQuery = "AddItemsToFridge";

const std::string FridgeDB::Connection::k_requestKey = "request";
const std::string FridgeDB::Connection::k_itemsKey = "items";
const std::string FridgeDB::Connection::k_successKey = "success";
const std::string FridgeDB::Connection::k_valuesKey = "values";

const nlohmann::json FridgeDB::Connection::k_emptyResponseJSON({
	{k_successKey, false},
});

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
	CompileInitializers();
	MaybeInitializeDB();
	CompileQueryStatements();
}

bool FridgeDB::Connection::CompileQueryStatements()
{
	using namespace FridgeDBHandlers;

	m_beginTransaction            = Statement(m_conn, beginTransactionStmtStr);
	m_endTransaction              = Statement(m_conn, endTransactionStmtStr);
	m_getItemsInFridge            = Statement(m_conn, getItemsInFridgeStmtStr);
	m_addItemsToFridgeInsertClass = Statement(m_conn, addItemsToFridgeInsertClassStmtStr);
	m_addItemsToFridgeUpdateClass = Statement(m_conn, addItemsToFridgeUpdateClassStmtStr);
	m_addItemsToFridgeInsertItem  = Statement(m_conn, addItemsToFridgeInsertItemStmtStr);

	return (
		m_beginTransaction.Ok() &&
		m_endTransaction.Ok() &&
		m_getItemsInFridge.Ok() &&
		m_addItemsToFridgeInsertClass.Ok() &&
		m_addItemsToFridgeUpdateClass.Ok() &&
		m_addItemsToFridgeInsertItem.Ok()
	);
}

bool FridgeDB::Connection::CompileInitializers()
{
	for (const auto &stmtStr : FridgeDBHandlers::initializers) {
		Statement stmt(m_conn, stmtStr);
		if (stmt.Ok()) {
			m_initializers.push_back(std::move(stmt));
		} else {
			std::cerr << "Failed to compile an initializer statement: " <<
				sqlite3_errmsg(m_conn) << std::endl <<
				stmtStr << std::endl;
			return false;
		}
	}
	return true;
}

void FridgeDB::Connection::MaybeInitializeDB()
{
	for (auto &initializer : m_initializers) {
		int rv;
		sqlite3_stmt *rawStmt = initializer.Get();

		for (;;) {
			rv = sqlite3_step(rawStmt);
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
		sqlite3_reset(rawStmt);
	}
}

std::string FridgeDB::Connection::Query(const std::string &requestString)
{
	JSONStaticReplies &replies = JSONStaticReplies::GetInstance();

	nlohmann::json request = nlohmann::json::parse(requestString, nullptr, false, true);
	if (request.is_discarded())
		return replies.GetNotJSONError();

	const auto &requestNameIt = request.find(k_requestKey);
	if (requestNameIt == request.cend())
		return replies.GetNoSuchRequestError();

	const std::string &requestName = *requestNameIt;
	
	if (requestName == k_getItemsInFridgeQuery) {
		return nlohmann::to_string(GetItemsInFridge());
	}
	else if (requestName == k_addItemsToFridgeQuery) {
		const auto &items = request.find(k_itemsKey);
		if (items == request.cend())
			return replies.GetItemsNotDefinedError();

		return nlohmann::to_string(AddItemsToFridge(*items));
	}
	return replies.GetNoSuchRequestError();
}

nlohmann::json FridgeDB::Connection::GetItemsInFridge()
{
	nlohmann::json reply(k_emptyResponseJSON);
	reply[k_valuesKey] = nlohmann::json::array();

	StatementIterator stmtIt(m_getItemsInFridge.Get(), k_getItemsInFridgeQuery);
	for (; !stmtIt.Done() && stmtIt.Ok(); ++stmtIt) {
		sqlite3_stmt *rawStmt = *stmtIt;

		std::string name(reinterpret_cast<const char *>(
			sqlite3_column_text(rawStmt, 0)));
		float amount = sqlite3_column_double(rawStmt, 1);
		int date = sqlite3_column_int(rawStmt, 2);
		int expireDate = sqlite3_column_int(rawStmt, 3);

		reply[k_valuesKey].push_back(nlohmann::json({
			{"name",       name},
			{"amount",     amount},
			{"date",       date},
			{"expireDate", expireDate},
		}));
	}
	if (stmtIt.Ok())
		reply[k_successKey] = true;

	return reply;
}

nlohmann::json FridgeDB::Connection::AddItemsToFridge(const nlohmann::json::array_t &items)
{
	for (const auto &item : items) {
		std::cout << nlohmann::to_string(item) << std::endl;
	}
	return nlohmann::json();
}

FridgeDB::Connection::~Connection()
{
	if (m_conn != nullptr)
		sqlite3_close(m_conn);
}

FridgeDB::Connection::Statement::Statement(Statement &&rhs)
{
	m_rawStmt = rhs.m_rawStmt;
	rhs.m_rawStmt = nullptr;
}

FridgeDB::Connection::Statement &FridgeDB::Connection::Statement::operator=(Statement &&rhs)
{
	if (this != &rhs) {
		if (m_rawStmt != nullptr) {
			sqlite3_finalize(m_rawStmt);
		}
		m_rawStmt = rhs.m_rawStmt;
		rhs.m_rawStmt = nullptr;
	}
	return *this;
}

FridgeDB::Connection::Statement::Statement(sqlite3 *conn,
	std::string_view stmtStr)
{
	std::string stmtStrCopy(stmtStr);
	int rv = sqlite3_prepare_v2(conn,
	                            stmtStrCopy.c_str(),
	                            stmtStrCopy.size(),
	                            &m_rawStmt,
	                            nullptr);
	if (rv != SQLITE_OK) {
		std::cerr << "Failed to compile statement:" << std::endl <<
			stmtStr << std::endl << 
			sqlite3_errmsg(conn) << std::endl;

		sqlite3_finalize(m_rawStmt);
		m_rawStmt = nullptr;
	}
}

FridgeDB::Connection::Statement::~Statement()
{
	if (m_rawStmt != nullptr)
		sqlite3_finalize(m_rawStmt);
}

FridgeDB::Connection::StatementIterator::StatementIterator(
	sqlite3_stmt *rawStmt, const std::string &requestName)
	: m_rawStmt(rawStmt)
	, m_requestName(requestName)
	, m_ok(true)
	, m_done(false)
{
	// Perform first step already, so the columns can be read right after
	// initializing the iterator
	++*this;
}

FridgeDB::Connection::StatementIterator &
	FridgeDB::Connection::StatementIterator::operator++()
{
	int rv;
again:
	rv = sqlite3_step(m_rawStmt);

	switch (rv) {
	case SQLITE_BUSY:
		goto again;
	case SQLITE_MISUSE:
		std::cerr << "SQLITE_MISUSE on " << m_requestName <<
			std::endl;
		m_ok = false;
		m_done = true;
		break;
	case SQLITE_DONE:
		m_done = true;
		break;
	case SQLITE_ROW:
		break;
	};
	return *this;
}

FridgeDB::Connection::StatementIterator::~StatementIterator()
{
	sqlite3_reset(m_rawStmt);
}

FridgeDB::JSONStaticReplies::JSONStaticReplies()
{
	nlohmann::json notJSONError = {{"message", "error: not JSON"}};
	nlohmann::json noSuchRequestError = {{"message", "error: no such request"}};
	nlohmann::json itemsNotDefinedError = {{"message", "error: items not defined"}};

	m_notJSONError = nlohmann::to_string(notJSONError);
	m_noSuchRequestError = nlohmann::to_string(noSuchRequestError);
	m_itemsNotDefinedError = nlohmann::to_string(itemsNotDefinedError);
}

FridgeDB::JSONStaticReplies &FridgeDB::JSONStaticReplies::GetInstance()
{
	static JSONStaticReplies instance;
	return instance;
}
