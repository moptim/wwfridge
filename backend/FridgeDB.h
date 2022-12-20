#ifndef FRIDGEDB_H_
#define FRIDGEDB_H_

#include <optional>
#include <string>
#include <sqlite3.h>

class FridgeDB {
public:
	class Connection;

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
	sqlite3 *m_conn;
};

#endif
