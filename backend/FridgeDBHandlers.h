#ifndef FRIDGEDBHANDLERS_H_
#define FRIDGEDBHANDLERS_H_

#include <array>
#include <string_view>

namespace FridgeDBHandlers {

struct FridgeDBHandler {
	std::string_view command;
	std::string_view statementString;
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

constexpr FridgeDBHandler getItemsInFridge = {"GetItemsInFridge",
	"hello world"};

constexpr FridgeDBHandler addItemToFridge = {"AddItemToFridge",
	"hello world"};

constexpr FridgeDBHandler delItemInFridge = {"DelItemInFridge",
	"hello world"};

constexpr std::array<FridgeDBHandler, 3> handlers {
	getItemsInFridge,
	addItemToFridge,
	delItemInFridge,
};

}

#endif
