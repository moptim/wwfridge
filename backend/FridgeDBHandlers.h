#ifndef FRIDGEDBHANDLERS_H_
#define FRIDGEDBHANDLERS_H_

#include "nlohmann/json.hpp"
#include <array>
#include <string_view>
#include <sqlite3.h>

namespace FridgeDBHandlers {

constexpr std::array<std::string_view, 4> initializers {
	"CREATE TABLE IF NOT EXISTS ItemClasses("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"name TEXT UNIQUE,"
		"unit TEXT,"
		"expireTime INTEGER);",

	"CREATE TABLE IF NOT EXISTS ItemAmounts("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"itemClassId INTEGER,"
		"amount REAL,"
		"date INTEGER);",

	"CREATE TABLE IF NOT EXISTS ShoppingList("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"itemClassId INTEGER,"
		"amount REAL,"
		"date INTEGER);",

	"CREATE VIEW IF NOT EXISTS GetItemsInFridge("
		"id, name, amount, unit, date, expireDate) "
	"AS SELECT object.id, class.name, object.amount, class.unit, object.date, object.date + class.expireTime "
	"FROM ItemClasses as class "
		"JOIN ItemAmounts as object ON class.id = object.itemClassId "
		"ORDER BY object.date + class.expireTime, class.name;",
};

static const std::string_view beginTransactionStmtStr =
	"BEGIN TRANSACTION;";

static const std::string_view endTransactionStmtStr =
	"END TRANSACTION;";

static const std::string_view getItemsInFridgeStmtStr =
	"SELECT * FROM GetItemsInFridge;";

static const std::string_view addItemsToFridgeInsertClassStmtStr =
	"INSERT OR IGNORE INTO ItemClasses (name, unit, expireTime) VALUES (?, ?, ?);";

static const std::string_view addItemsToFridgeUpdateClassStmtStr =
	"UPDATE ItemClasses SET unit = ?, expireTime = ? WHERE name = ? RETURNING id;";

static const std::string_view addItemsToFridgeInsertItemStmtStr =
	"INSERT INTO ItemAmounts (itemClassId, amount, date) VALUES (?, ?, ?);";

}

#endif
