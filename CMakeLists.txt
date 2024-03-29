
# CMAKE_INSTALL_PREFIX defaults to /usr/local.

cmake_minimum_required(VERSION 3.1)
project("mpw-shell")
set (PROJECT_TYPE "CXX")
set (PROJECT_NAME "MPW Shell")


# -std=c++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)
set(CMAKE_CXX_EXTENSIONS FALSE)


#
# build config.h
#

INCLUDE (CheckFunctionExists)
INCLUDE (CheckLibraryExists)

SET(CMAKE_EXTRA_INCLUDE_FILES stdio.h)
CHECK_FUNCTION_EXISTS(dprintf HAVE_DPRINTF)
SET(CMAKE_EXTRA_INCLUDE_FILES)

CHECK_LIBRARY_EXISTS(edit readline "" HAVE_LIBEDIT)
CHECK_LIBRARY_EXISTS(readline readline "" HAVE_LIBREADLINE)
CHECK_LIBRARY_EXISTS(history add_history "" HAVE_LIBHISTORY)

CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in ${CMAKE_CURRENT_BINARY_DIR}/config.h)


add_compile_options(-g -Wall -Wno-unused-variable -Wno-multichar -O1)

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	add_compile_options(-Wno-unused-const-variable)
endif()


if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
	add_compile_options(-Wno-unused-but-set-variable)
endif()


include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

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


#RAGEL_TARGET(phase1 phase1.rl phase1.cpp COMPILE_FLAGS "-p -G2")
RAGEL_TARGET(phase2 phase2.rl phase2.cpp COMPILE_FLAGS "-p -G2")
RAGEL_TARGET(pathnames pathnames.rl pathnames.cpp COMPILE_FLAGS "-p -G2")
RAGEL_TARGET(mpw-shell-token mpw-shell-token.rl mpw-shell-token.cpp COMPILE_FLAGS "-p -G2")
RAGEL_TARGET(mpw-shell-expand mpw-shell-expand.rl mpw-shell-expand.cpp COMPILE_FLAGS "-p -G2")
RAGEL_TARGET(mpw-shell-quote mpw-shell-quote.rl mpw-shell-quote.cpp COMPILE_FLAGS "-p -G2")
RAGEL_TARGET(value value.rl value.cpp COMPILE_FLAGS "-p -G2")


# need to copy all OUTPUT file to the build dir
# add_custom_command(
# 	OUTPUT phase3.cpp phase3.h phase3.out
# 	COMMAND lemon++ phase3.lemon
# 	COMMAND mv -f phase3.cpp phase3.h phase3.out ${CMAKE_CURRENT_BINARY_DIR}/
# 	MAIN_DEPENDENCY phase3.lemon 
# 	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
# )

find_program(LEMON_PLUSPLUS NAMES lemon++)
if (LEMON_PLUSPLUS)
	add_custom_command(
		OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/phase3.cpp ${CMAKE_CURRENT_SOURCE_DIR}/phase3.h
		#COMMAND cp -f "${CMAKE_CURRENT_SOURCE_DIR}/parser.lemon" "parser.lemon"
		COMMAND lemon++ phase3.lemon
		MAIN_DEPENDENCY phase3.lemon
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	)
endif()


add_executable(mpw-shell mpw-shell.cpp  mpw-shell-token.cpp mpw-shell-expand.cpp 
	mpw-shell-parser.cpp mpw_parser.cpp value.cpp mpw-shell-quote.cpp mpw-regex.cpp
	phase1.cpp phase2.cpp phase3.cpp command.cpp environment.cpp builtins.cpp 
	pathnames.cpp
	macroman.cpp
	cxx/mapped_file.cpp
	cxx/filesystem.cpp
	cxx/path.cpp
	cxx/directory_iterator.cpp
)

#
# -ledit includes history stuff.  gnu -lreadline does not.
#
if(HAVE_LIBEDIT)
	target_link_libraries(mpw-shell edit)
elseif(HAVE_LIBREADLINE)
	target_link_libraries(mpw-shell readline)
	if (HAVE_LIBHISTORY)
		target_link_libraries(mpw-shell history)
	endif()
endif()



# create a symlink for mpw-make
add_custom_command(
  TARGET mpw-shell
  POST_BUILD
  COMMAND ln;-sf;mpw-shell;mpw-make
  COMMENT "ln -s mpw-shell mpw-make"
)

# install...

install(
  PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/mpw-shell
    ${CMAKE_CURRENT_BINARY_DIR}/mpw-make
  DESTINATION bin
)
