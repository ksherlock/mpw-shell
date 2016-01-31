#ifndef __phase1_h__
#define __phase1_h__


#include <string>
#include <functional>

class phase1 {
	
public:
	typedef std::function<void(std::string &&)> pipe_function;

	phase1();

	void process(const unsigned char *begin, const unsigned char *end, bool final = false);

	void process(const char *begin, const char *end, bool final = false) {
		process((const unsigned char *)begin, (const unsigned char *)end, final);
	}

	void process(const std::string &s) { process(s.data(), s.data() + s.size()); }

	void finish() { const char *tmp = ""; process(tmp, tmp, true); }

	void reset();


	phase1 &operator >>= (pipe_function f) { pipe_to = f; return *this; }


	template<class F>
	phase1 &operator >>= (F &f) { 
		using std::placeholders::_1;
		pipe_to = std::bind(&F::operator(), &f, _1);
		return *this;
	}



private:
	std::string scratch;
	pipe_function pipe_to;
	int line = 1;
	int cs = 0;
};

#endif
