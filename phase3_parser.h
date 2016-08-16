
#ifndef __phase3_h__
#define __phase3_h__

#include <string>
#include <vector>
#include <memory>

#include "command.h"
#include <lemon_base.h>

class phase3 : public lemon_base<std::string> {

public:
	static std::unique_ptr<phase3> make();

	virtual void syntax_error(int yymajor, std::string &yyminor) override final;
	virtual void parse_accept() override final;
	virtual void parse_failure() override final;

	bool continuation() const;

	//void parse(int type, std::string &&s) override final;

private:

	phase3(const phase3 &) = delete;
	phase3(phase3 &&) = delete;

	phase3& operator=(const phase3 &) = delete;
	phase3& operator=(phase3 &&) = delete;

protected:
	// these need to be accessible to the lemon-generated parser.
	phase3() = default;

	command_ptr_vector command_queue;
	bool error = false;

	friend class mpw_parser;
};

#endif