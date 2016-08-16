#ifndef __phase1_h__
#define __phase1_h__


#include <string>
#include <functional>

class phase1 {
	
public:

	typedef std::function<void(std::string &&)> next_function_type;

	phase1() = default;

	void parse(const unsigned char *begin, const unsigned char *end);
	void parse(const std::string &s) { parse((const unsigned char *)s.data(), (const unsigned char *)s.data() + s.size()); }
	void finish();

	void reset();
	void abort() { reset(); }

	bool continuation() const { return multiline; }

	void set_next(next_function_type &&fx) { _then = std::move(fx); }

private:

	int process(unsigned char, int);
	void flush();

	std::string scratch;
	int line = 1;
	int cs = 0;
	bool multiline = false;

	next_function_type _then;
};

#endif
