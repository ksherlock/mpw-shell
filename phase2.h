
#ifndef __phase2_h__
#define __phase2_h__

#include <string>
#include <vector>
#include <functional>
#include <memory>

#include "command.h"
#include "lemon_base.h"

//typedef std::unique_ptr<struct command> command_ptr;
//typedef std::vector<command_ptr> command_ptr_vector;

class phase2_parser : public lemon_base<std::string> {

public:
	static std::unique_ptr<phase2_parser> make();

	virtual void syntax_error(int yymajor, std::string &yyminor) override final;
	virtual void parse_accept() override final;
	virtual void parse_failure() override final;

	bool continuation() const;

private:
	friend class phase2;

	phase2_parser(const phase2_parser &) = delete;
	phase2_parser(phase2_parser &&) = delete;

	phase2_parser& operator=(const phase2_parser &) = delete;
	phase2_parser& operator=(phase2_parser &&) = delete;

protected:
	// these need to be accessible to the lemon-generated parser.
	phase2_parser() = default;

	command_ptr_vector command_queue;
	bool error = false;
};


class phase2  {
	
public:
	typedef std::function<void(command_ptr_vector &&)> pipe_function;

	phase2();
	phase2(const phase2 &) = delete;
	phase2(phase2 &&) = default;

	phase2 & operator=(const phase2 &) = delete;
	phase2 & operator=(phase2 &&) = default;

	void operator()(const std::string &line) { process(line); }
	void process(const std::string &line);
	void finish();

	phase2 &operator >>=(pipe_function f) { pipe_to = f; return *this; }

	template<class F>
	phase2 &operator >>= (F &f) { 
		using std::placeholders::_1;
		pipe_to = std::bind(&F::operator(), &f, _1);
		return *this;
	}

	bool continuation() const {
		return parser ? parser->continuation() : false;
	}

	void abort();

private:

	void parse(int, std::string &&);


	std::unique_ptr<phase2_parser> parser;

	std::string scratch;
	int type = 0;
	int pcount = 0;

	pipe_function pipe_to;

	void flush();
	bool special();
	int classify();
	void exec();


};


#endif
