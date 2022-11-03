#ifndef __mpw_error_h__
#define __mpw_error_h__

#include <stdexcept>
#include <system_error>

class mpw_error : public std::runtime_error {
public:
	mpw_error(int status, const std::string &s) : std::runtime_error(s), _status(status)
	{}

	mpw_error(int status, const char *s) : std::runtime_error(s), _status(status)
	{}

	int status() const noexcept { return _status; }
private:
	int _status;
};

class execution_of_input_terminated : public mpw_error {
public:
	execution_of_input_terminated(int status = -9) :
	mpw_error(status, "MPW Shell - Execution of input Terminated.")
	{}
};


class break_error : public mpw_error {
public:
	break_error(int status = -3) : 
	mpw_error(status, "MPW Shell - Break must be within for or loop.")
	{}
};

class continue_error : public mpw_error {
public:
	continue_error(int status = -3) : 
	mpw_error(status, "MPW Shell - Continue must be within for or loop.")
	{}
};

class estring_error: public mpw_error {
public:
	estring_error(int status = -3) : 
	mpw_error(status, "MPW Shell - `s must occur in pairs.")
	{}
};

class vstring_error: public mpw_error {
public:
	vstring_error(int status = -3) : 
	mpw_error(status, "MPW Shell - {s must occur in pairs.")
	{}
};

class sstring_error: public mpw_error {
public:
	sstring_error(int status = -3) : 
	mpw_error(status, "MPW Shell - 's must occur in pairs.")
	{}
};

class dstring_error: public mpw_error {
public:
	dstring_error(int status = -3) : 
	mpw_error(status, "MPW Shell - \"s must occur in pairs.")
	{}
};

class fsstring_error: public mpw_error {
public:
	fsstring_error(int status = -3) : 
	mpw_error(status, "MPW Shell - /s must occur in pairs.")
	{}
};

class bsstring_error: public mpw_error {
public:
	bsstring_error(int status = -3) : 
	mpw_error(status, "MPW Shell - \\s must occur in pairs.")
	{}
};


/*
  these are used for flow-control.
  they do not inherit from std::exception to prevent being caught
  by normal handlers.
*/

struct break_command_t {};
struct continue_command_t {};
struct exit_command_t { int value = 0; };
struct quit_command_t {};

#endif
