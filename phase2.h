
#ifndef __phase2_h__
#define __phase2_h__

#include <string>
#include <functional>

class phase2  {
	
public:

	typedef std::function<void(int, std::string &&)> next_function_type;

	phase2() = default;

	void parse(std::string &&);
	void finish();

	void reset();
	void abort() { reset(); }

	bool continuation() const { return false; }

	void set_next(next_function_type &&fx) { _then = std::move(fx); }

private:

	void parse(int type, std::string &&s);

	std::string scratch;
	int type = 0;
	int pcount = 0;


	void flush();
	bool special();
	int classify();
	void exec();

	next_function_type _then;
};


#endif
