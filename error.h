#ifndef __mpw_error_h__
#define __mpw_error_h__

#include <stdexcept>
#include <system_error>

class execution_of_input_terminated : public std::runtime_error {
public:
	execution_of_input_terminated(int status = -9) : 
	std::runtime_error("MPW Shell - Execution of input Terminated."), _status(status)
	{}
	constexpr int status() const noexcept { return _status; }
private:
	int _status;
};

#endif