
# CMAKE_INSTALL_PREFIX defaults to /usr/local.

cmake_minimum_required(VERSION 2.6)
project("mpw-shell")
set (PROJECT_TYPE "CXX")
set (PROJECT_NAME "MPW Shell")

set(CMAKE_CXX_FLAGS "-g -Wall -Wno-unused-variable -Wno-multichar -O1")

if(${CMAKE_CXX_COMPILER_ID} MATCHES "Clang")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-const-variable ")
endif()


if(${CMAKE_CXX_COMPILER_ID} MATCHES "GNU")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-unused-but-set-variable")
endif()



add_definitions(-I ${CMAKE_SOURCE_DIR}/ -I ${CMAKE_CURRENT_BINARY_DIR}/ )

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
add_custom_command(
	OUTPUT phase2-parser.cpp phase2-parser.h phase2-parser.out
	COMMAND lemon++ -Tlempar.cxx phase2-parser.lemon
	COMMAND mv -f phase2-parser.cpp phase2-parser.h phase2-parser.out ${CMAKE_CURRENT_BINARY_DIR}/
	MAIN_DEPENDENCY phase2-parser.lemon 
	DEPENDS lempar.cxx
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)


add_executable(mpw-shell mpw-shell.cpp  mpw-shell-token.cpp mpw-shell-expand.cpp 
	mpw-shell-parser.cpp value.cpp mpw-shell-quote.cpp 
	phase1.cpp phase2.cpp phase2-parser.cpp command.cpp environment.cpp builtins.cpp 
	pathnames.cpp
	macroman.cpp
	cxx/mapped_file.cpp
	cxx/filesystem.cpp
	cxx/path.cpp
	cxx/directory_iterator.cpp
)

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
  PROGRAMS
    ${CMAKE_CURRENT_BINARY_DIR}/mpw-shell
    ${CMAKE_CURRENT_BINARY_DIR}/mpw-make
  DESTINATION bin
)
