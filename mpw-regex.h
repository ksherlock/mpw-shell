#ifndef __mpw_regex_h__
#define __mpw_regex_h__

#include "environment.h"

#include <string>
#include <regex>

class mpw_regex {

public:

	mpw_regex(const std::string &s, bool slash);

	mpw_regex(const mpw_regex &) = default;
	// mpw_regex(mpw_regex &&) = default;

	~mpw_regex() = default;


	mpw_regex &operator=(const mpw_regex &) = default;
	// mpw_regex &operator=(mpw_regex &&) = default;

	bool match(const std::string &, class Environment &);
	bool match(const std::string &);

	static bool is_glob(const std::string &s);

private:
	typedef std::string::const_iterator iterator;


	void convert_re(const std::string &, bool slash);

	iterator convert_re(iterator iter, iterator end, std::string &accumulator, unsigned char term);
	iterator convert_re_repeat(iterator iter, iterator end, std::string &accumulator);
	iterator convert_re_set(iterator iter, iterator end, std::string &accumulator);
	iterator convert_re_capture(iterator iter, iterator end, std::string &accumulator);


	std::regex re;
	std::string key;
	int capture_map[10] = {}; // map mpw capture number to ecma group
	int num_captures = 0;

};

#endif
