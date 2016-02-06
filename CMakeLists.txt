
# CMAKE_INSTALL_PREFIX defaults to /usr/local.

cmake_minimum_required(VERSION 2.6)
project("mpw-shell")
set (PROJECT_TYPE "CXX")
set (PROJECT_NAME "MPW Shell")

set(CMAKE_CXX_FLAGS "-g -Wall -Wno-unused-variable -Wno-multichar")

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-const-variable ")
endif()


if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")
endif()



add_definitions(-I ${CMAKE_SOURCE_DIR}/)

# from https://github.com/gsauthof/cmake-ragel
macro(RAGEL_TARGET Name Input Output)
	set(RAGEL_EXECUTABLE "ragel")
	set(RAGEL_TARGET_usage
	          "RAGEL_TARGET(<Name> <Input> <Output> [COMPILE_FLAGS <string>]")
	if(${ARGC} GREATER 3)
	  if(${ARGC} EQUAL 5)
	    if("${ARGV3}" STREQUAL "COMPILE_FLAGS")
	      set(RAGEL_EXECUTABLE_opts  "${ARGV4}")
	      separate_arguments(RAGEL_EXECUTABLE_opts)
	    else()
	      message(SEND_ERROR ${RAGEL_TARGET_usage})
	    endif()
	  else()
	    message(SEND_ERROR ${RAGEL_TARGET_usage})
	  endif()
	endif()

	add_custom_command(OUTPUT ${Output}
	  COMMAND ${RAGEL_EXECUTABLE}
	  ARGS    ${RAGEL_EXECUTABLE_opts} -o${CMAKE_CURRENT_BINARY_DIR}/${Output} ${Input}
	  DEPENDS ${Input}
	  COMMENT
	      "[RAGEL][${Name}] Compiling state machine with Ragel ${RAGEL_VERSION}"
	  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

	set(RAGEL_${Name}_DEFINED       TRUE)
	set(RAGEL_${Name}_OUTPUTS       ${Output})
	set(RAGEL_${Name}_INPUT         ${Input})
	set(RAGEL_${Name}_COMPILE_FLAGS ${RAGEL_EXECUTABLE_opts})
endmacro()





#add_custom_command(
#	OUTPUT mpw-shell-read.cpp
#	COMMAND ragel -p -G2 -o mpw-shell-read.cpp "${CMAKE_CURRENT_SOURCE_DIR}/mpw-shell-read.rl"
#	MAIN_DEPENDENCY mpw-shell-read.rl 
#)

RAGEL_TARGET(mpw-shell-read mpw-shell-read.rl mpw-shell-read.cpp COMPILE_FLAGS "-p -G2")
RAGEL_TARGET(phase1 phase1.rl phase1.cpp COMPILE_FLAGS "-p -G2")
RAGEL_TARGET(phase2 phase2.rl phase2.cpp COMPILE_FLAGS "-p -G2")

# need to copy all OUTPUT file to the build dir
add_custom_command(
	OUTPUT phase2-parser.cpp phase2-parser.h
	COMMAND lemon++ -Tlempar.cxx phase2-parser.lemon
	COMMAND cp -p phase2-parser.cpp phase2-parser.h ${CMAKE_CURRENT_BINARY_DIR}/
	MAIN_DEPENDENCY phase2-parser.lemon 
	DEPENDS lempar.cxx
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
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


# mpw-shell-execute.cpp mpw-shell-builtins.cpp mpw-shell-read.cpp
add_executable(mpw-shell mpw-shell.cpp  mpw-shell-token.cpp mpw-shell-expand.cpp 
	mpw-shell-parser.cpp value.cpp mpw-shell-quote.cpp 
	phase1.cpp phase2.cpp phase2-parser.cpp command.cpp environment.cpp builtins.cpp mapped_file.cpp)

target_link_libraries(mpw-shell edit)

# all this for -std=c++14
set_property (TARGET mpw-shell PROPERTY CXX_STANDARD 14)
set_property (TARGET mpw-shell PROPERTY CXX_STANDARD_REQUIRED TRUE)
set_property (TARGET mpw-shell PROPERTY CXX_EXTENSIONS FALSE)

# create a symlink for mpw-make
add_custom_command(
  TARGET mpw-shell
  POST_BUILD
  COMMAND ln;-sf;mpw-shell;mpw-make
  COMMENT "ln -s mpw-shell mpw-make"
)

# install...

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/mpw-shell
    ${CMAKE_CURRENT_BINARY_DIR}/mpw-make
  DESTINATION bin
)
