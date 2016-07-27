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

	constexpr int status() const noexcept { return _status; }
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

#endif