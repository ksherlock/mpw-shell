
#ifndef __phase2_h__
#define __phase2_h__

#include <string>
#include <vector>
#include <functional>
#include <memory>

typedef std::unique_ptr<struct command> command_ptr;
typedef std::vector<command_ptr> command_ptr_vector;

class phase2  {
	
public:
	typedef std::function<void(command_ptr &&)> pipe_function;

	void process(const std::string &line);
	void finish();

	virtual void syntax_error();
	virtual void parse_accept();
	virtual void parse(int, std::string &&);

	phase2 &operator >>=(pipe_function f) { pipe_to = f; return *this; }

private:

	std::string scratch;
	int type = 0;
	bool error = false;
	bool immediate = false;

	pipe_function pipe_to;

	void flush();
	bool special();
	void classify();
	void exec();

	command_ptr_vector command_queue;
};

#endif
