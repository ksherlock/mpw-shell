

#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <system_error>
#include <algorithm>

#include <stdio.h>

#include "mpw-shell.h"
#include "error.h"

%%{
	
	machine expand;
	alphtype unsigned char;

	action push { scratch.push_back(fc); }
	action vinit { /* vinit */ ev.clear(); xcs = fcurs; fgoto vstring_state; }
	action vpush { /* vpush */ ev.push_back(fc); }
	action vfinish0 { /* vfinish0 */ fnext *xcs; }
	action vfinish1 {
		/* vfinish1 */
		auto iter = env.find(ev);
		if (iter != env.end()) {
			const std::string &s = iter->second;
			scratch.append(s);
		}
		fgoto *xcs;
	}
	action vfinish2 {
		/* vfinish2 */
		auto iter = env.find(ev);
		if (iter != env.end()) {
			// quote special chars...
			const std::string &s = iter->second;
			for (auto c : s) {
				if (c == '\'' || c == '"' ) scratch.push_back(escape);
				scratch.push_back(c);
			}
		}
		fgoto *xcs;
	}

	action einit { /* einit */ ev.clear(); xcs = fcurs; fgoto estring_state; }
	action epush { /* epush */ ev.push_back(fc); }
	action efinish1 {
		/* efinish1 */

		/*
		throw std::runtime_error("MPW Shell - `...` not yet supported.");
		*/

		std::string s = subshell(ev, env, fds);
		scratch.append(s);

		fgoto *xcs;
	}
	action efinish2 {
		/* efinish2 */

		std::string s = subshell(ev, env, fds);
		for (auto c : s) {
			if (c == '\'' || c == '"' ) scratch.push_back(escape);
			scratch.push_back(c);
		}

		fgoto *xcs;
	}

	action vstring_error{
		throw vstring_error();
	}

	action estring_error{
		throw estring_error();
	}


	escape = 0xb6;
	char = any - escape - ['"{`];
	escape_seq = escape any;

	schar = [^'];
	sstring = ['] schar** ['];


	vchar = [^}] $vpush;
	vchar1 = [^{}] $vpush;

	vstring0 = '}' @vfinish0;
	vstring1 = vchar1 vchar** '}' @vfinish1;
	vstring2 = '{' vchar** '}}' @vfinish2;

	vstring_state := (vstring0 | vstring1 | vstring2) $err(vstring_error);
	vstring = '{' $vinit;

	echar = (escape_seq | any - escape - [`]) $epush;

	estring1 = echar+ '`' @efinish1;
	estring2 = '`' echar* '``' @efinish2;

	estring_state := (estring1 | estring2) $err(estring_error);
	estring = '`' $einit;



	dchar = escape_seq | estring | vstring | (any - escape - [`{"]);
	dstring = ["] dchar** ["] ;


	main := (
		  escape_seq $push
		| sstring $push
		| dstring $push
		| vstring
		| estring
		| char $push
	)**;


}%%

namespace {
	%% write data;


std::string subshell(const std::string &s, Environment &env, const fdmask &fds) {
	

	char temp[32] = "/tmp/mpw-shell-XXXXXXXX";

	int fd = mkstemp(temp);
	unlink(temp);

	fdset new_fds;
	new_fds.set(1, fd);

	int rv = 0;
	env.indent_and([&](){

		rv = read_string(env, s, new_fds | fds);

	});

	std::string tmp;
	lseek(fd, 0, SEEK_SET);
	for(;;) {
		uint8_t buffer[1024];
		ssize_t len = read(fd, buffer, sizeof(buffer));
		if (len == 0) break;
		if (len < 0) {
			if (errno == EINTR) continue;
			throw std::system_error(errno, std::system_category(), "read");
		}
		tmp.append(buffer, buffer + len);
	}
	std::transform(tmp.begin(), tmp.end(), tmp.begin(), [](uint8_t x){
		if (x == '\r' || x == '\n') x = ' ';
		return x;
	});
	return tmp;
}

}

std::string expand_vars(const std::string &s, Environment &env, const fdmask &fds) {
	if (s.find_first_of("{`", 0, 2) == s.npos) return s;

	int cs;
	int xcs;

	const unsigned char *p = (const unsigned char *)s.data();
	const unsigned char *pe = p + s.size();
	const unsigned char *eof = pe;

	std::string scratch;
	std::string ev;

	scratch.reserve(s.size());
	%% write init;
	%% write exec;

	return scratch;
}
