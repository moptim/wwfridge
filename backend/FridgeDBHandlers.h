#ifndef FRIDGEDBHANDLERS_H_
#define FRIDGEDBHANDLERS_H_

#include "nlohmann/json.hpp"
#include <array>
#include <string_view>
#include <sqlite3.h>

namespace FridgeDBHandlers {

struct FridgeDBHandler {
	std::string_view command;
	std::string_view statementString;
	std::function<int(sqlite3_stmt *)> BindParams;
	std::function<nlohmann::json(sqlite3_stmt *)> RowCallback;
	sqlite3_stmt *raw_stmt;
};

constexpr std::array<std::string_view, 4> initializers {
	"CREATE TABLE IF NOT EXISTS ItemClasses("
		"id INTEGER PRIMARY KEY AUTOINCREMENT,"
		"name TEXT UNIQUE,"
		"unit TEXT,"
		"expireTime INTEGER);",

	"CREATE TABLE IF NOT EXISTS ItemAmounts("
		"itemId INTEGER,"
		"amount INTEGER,"
		"date INTEGER);",

	"CREATE TABLE IF NOT EXISTS ShoppingList("
		"itemId INTEGER,"
		"amount INTEGER,"
		"date INTEGER);",

	"CREATE VIEW IF NOT EXISTS GetItemsInFridge("
		"name, amount, date, expireDate) "
	"AS SELECT class.name, object.amount, object.date, object.date + class.expireTime "
	"FROM ItemClasses as class "
		"JOIN ItemAmounts as object ON class.id = object.itemId "
		"ORDER BY object.date + class.expireTime, class.name;",
};

const FridgeDBHandler getItemsInFridge = {
	.command = "GetItemsInFridge",
	.statementString = "SELECT * FROM GetItemsInFridge;",
	.BindParams  = [](sqlite3_stmt *raw_stmt) -> int {
		return 0;
	},
	.RowCallback = [](sqlite3_stmt *raw_stmt) -> nlohmann::json {
		std::string name(reinterpret_cast<const char *>(sqlite3_column_text(raw_stmt, 0)));
		float amount = sqlite3_column_double(raw_stmt, 1);
		int date = sqlite3_column_int(raw_stmt, 2);
		int expireDate = sqlite3_column_int(raw_stmt, 3);

		return nlohmann::json({
			{"name",       name},
			{"amount",     amount},
			{"date",       date},
			{"expireDate", expireDate},
		});
	},
};

const FridgeDBHandler addItemToFridge = {
	.command = "AddItemToFridge",
	.statementString = "hello world",
};

const FridgeDBHandler delItemInFridge = {
	.command = "DelItemInFridge",
	.statementString = "hello world",
};

const std::array<FridgeDBHandler, 3> handlers {
	getItemsInFridge,
	addItemToFridge,
	delItemInFridge,
};

}

#endif
