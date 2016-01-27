#include <string>

bool must_quote(const std::string &s){
%%{
	
	machine must_quote;
	alphtype unsigned char;

	quotable = (
		[ \t\r\n]
		|
		0x00
		|
		[0x80-0xff]
		|
		[+#;&|()'"/\\{}`?*<>]
		|
		'-'
		|
		'['
		|
		']'
	);

	#simpler just to say what's ok.
	normal = [A-Za-z0-9_.:];

	main := 
		(
		normal
		|
		(any-normal) ${return true;}
		)*
	;
}%%

	%%write data;

	int cs;
	const unsigned char *p = (const unsigned char *)s.data();
	const unsigned char *pe = (const unsigned char *)s.data() + s.size();
	const unsigned char *eof = nullptr;

	%%write init;
	%%write exec;
	return false;
}

#if 0
std::string quote(const std::string &s) {
	std::string tmp(s);
	return quote(std::move(tmp));
}
#endif

std::string quote(const std::string &s) {
	const char q = '\'';
	const char *escape_q = "'\xd8''";

	if (!must_quote(s)) return s;

	std::string out;
	out.reserve(s.length() + (s.length() >> 1));
	out.push_back(q);

	for (char c : s) {
		if (c == q) {
			out.append(escape_q);
		} else
			out.push_back(c);
	}

	out.push_back(q);
	return out;
}
