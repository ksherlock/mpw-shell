#ifndef __mpw_parser__
#define __mpw_parser__

#include <string>
#include <memory>

#include "environment.h"
#include "fdset.h"

#include "phase1.h"
#include "phase2.h"

class mpw_parser {

public:

	mpw_parser(Environment &e, fdmask fds = fdmask(), bool interactive = false);
	mpw_parser(Environment &e, bool interactive) : mpw_parser(e, fdmask(), interactive)
	{}

	~mpw_parser();

	void parse(const void *begin, const void *end);
	void parse(const std::string &s) {
		parse(s.data(), s.data() + s.size());
	}
	void parse(const void *begin, size_t length) {
		parse(begin, (const char *)begin + length);
	}

	void reset();
	void abort();
	void finish();

	bool continuation() const;

private:

	mpw_parser& operator=(const mpw_parser &) = delete;
	mpw_parser& operator=(mpw_parser &&) = delete;

	mpw_parser(const mpw_parser &) = delete;
	mpw_parser(mpw_parser &&) = delete;

	void execute();

	Environment &_env;
	fdmask _fds;
	bool _interactive = false;
	bool _abort = false;

	phase1 _p1;
	phase2 _p2;
	std::unique_ptr<class phase3> _p3;

};


#endif