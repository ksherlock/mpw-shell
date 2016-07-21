#ifndef __phase1_h__
#define __phase1_h__


#include <string>
#include <functional>

class phase1 {
	
public:
	typedef std::function<void(std::string &&)> pipe_function;

	phase1() = default;

	void process(const unsigned char *begin, const unsigned char *end, bool final = false);

	void process(const char *begin, const char *end, bool final = false) {
		process((const unsigned char *)begin, (const unsigned char *)end, final);
	}

	void process(const std::string &s, bool final = false);// { process(s.data(), s.data() + s.size(), final); }

	void finish();// { const char *tmp = "\n"; process(tmp, tmp+1, true); }

	void reset();
	void abort() { reset(); }


	phase1 &operator >>= (pipe_function f) { pipe_to = f; return *this; }


	template<class F>
	phase1 &operator >>= (F &f) { 
		using std::placeholders::_1;
		pipe_to = std::bind(&F::operator(), &f, _1);
		return *this;
	}



private:

	int process(unsigned char, int);
	void flush();

	std::string scratch;
	pipe_function pipe_to;
	int line = 1;
	int cs = 0;
	bool multiline = false;
};

#endif
