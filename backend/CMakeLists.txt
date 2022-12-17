cmake_minimum_required(VERSION 3.14)
project(wwfridge_backend VERSION 0.1)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

add_executable(
	wwfridge_backend
	main.cpp
	WebSocketIF.cpp
	FridgeDB.cpp
)
target_include_directories(
	wwfridge_backend PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/uWebSockets/src/
	${CMAKE_CURRENT_SOURCE_DIR}/uWebSockets/uSockets/src/
)

find_package(SQLite3 REQUIRED)
find_package(ZLIB REQUIRED)

add_custom_command(
	OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/uWebSockets/uSockets/uSockets.a
	COMMAND make
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/uWebSockets/uSockets/
)

add_custom_target(
	uSockets
	DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/uWebSockets/uSockets/uSockets.a
)

target_link_libraries(
	wwfridge_backend
	${CMAKE_CURRENT_SOURCE_DIR}/uWebSockets/uSockets/uSockets.a
	sqlite3
	z
)

add_dependencies(wwfridge_backend uSockets)
