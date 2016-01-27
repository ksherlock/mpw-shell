
cmake_minimum_required(VERSION 2.6)
project("mpw-shell")
set (PROJECT_TYPE "CXX")
set (PROJECT_NAME "MPW Shell")


set(CMAKE_CXX_FLAGS "-std=c++14 -g -Wall -Wno-unused-variable")

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	#set(CMAKE_CXX_COMPILER "clang++")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-const-variable -Wno-multichar")
endif()


if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")
endif()



add_definitions(-I ${CMAKE_SOURCE_DIR}/)


add_custom_command(
	OUTPUT mpw-shell-read.cpp
	COMMAND ragel -p -G2 -o mpw-shell-read.cpp "${CMAKE_CURRENT_SOURCE_DIR}/mpw-shell-read.rl"
	MAIN_DEPENDENCY mpw-shell-read.rl 
)

add_custom_command(
	OUTPUT mpw-shell-expand.cpp
	COMMAND ragel -p -G2 -o mpw-shell-expand.cpp "${CMAKE_CURRENT_SOURCE_DIR}/mpw-shell-expand.rl"
	MAIN_DEPENDENCY mpw-shell-expand.rl 
)

add_custom_command(
	OUTPUT mpw-shell-token.cpp
	COMMAND ragel -p -G2 -o mpw-shell-token.cpp "${CMAKE_CURRENT_SOURCE_DIR}/mpw-shell-token.rl"
	MAIN_DEPENDENCY mpw-shell-token.rl 
)

add_custom_command(
	OUTPUT mpw-shell-command.cpp
	COMMAND ragel -p -G2 -o mpw-shell-command.cpp "${CMAKE_CURRENT_SOURCE_DIR}/mpw-shell-command.rl"
	MAIN_DEPENDENCY mpw-shell-command.rl 
)

add_custom_command(
	OUTPUT value.cpp
	COMMAND ragel -p -G2 -o value.cpp "${CMAKE_CURRENT_SOURCE_DIR}/value.rl"
	MAIN_DEPENDENCY value.rl 
)

add_custom_command(
	OUTPUT mpw-shell-quote.cpp
	COMMAND ragel -p -G2 -o mpw-shell-quote.cpp "${CMAKE_CURRENT_SOURCE_DIR}/mpw-shell-quote.rl"
	MAIN_DEPENDENCY mpw-shell-quote.rl 
)




add_executable(mpw-shell mpw-shell.cpp mpw-shell-read.cpp mpw-shell-token.cpp mpw-shell-expand.cpp 
	mpw-shell-execute.cpp mpw-shell-builtins.cpp mpw-shell-parser.cpp value.cpp mpw-shell-quote.cpp)

