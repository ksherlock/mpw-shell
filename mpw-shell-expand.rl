

#include <vector>
#include <string>
#include <unordered_map>
#include <stdexcept>

#include <stdio.h>

#include "mpw-shell.h"
#include "error.h"

%%{
	
	machine expand;
	alphtype unsigned char;

	action push { scratch.push_back(fc); }
	action vinit { /* vinit */ ev.clear(); xcs = fcurs; fnext vstring_state; }
	action vpush { /* vpush */ ev.push_back(fc); }
	action vfinish0 { /* vfinish0 */ fnext *xcs; }
	action vfinish1 {
		/* vfinish1 */
		fnext *xcs; 
		auto iter = env.find(ev);
		if (iter != env.end()) {
			const std::string &s = iter->second;
			scratch.append(s);
		}
	}
	action vfinish2 {
		/* vfinish2 */
		fnext *xcs;
		auto iter = env.find(ev);
		if (iter != env.end()) {
			// quote special chars...
			const std::string &s = iter->second;
			for (auto c : s) {
				if (c == '\'' || c == '"' ) scratch.push_back(escape);
				scratch.push_back(c);
			}
		}
	}

	action einit{ /* einit */ ev.clear(); xcs = fcurs; fnext estring_state; }
	action epush{ /* epush */ ev.push_back(fc); }
	action efinish1{
		/* efinish1 */
		fnext *xcs;
		throw std::runtime_error("MPW Shell - `...` not yet supported.");
	}
	action efinish2{
		/* efinish2 */
		fnext *xcs;
		throw std::runtime_error("MPW Shell - `...` not yet supported.");
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



	dchar = escape_seq $push | estring | vstring | (any - escape - [`{"]) $push;
	dstring = ["] dchar** ["];


	main := (
		  escape_seq $push
		| sstring $push
		| dstring
		| vstring
		| estring
		| char $push
	)**;


}%%

namespace {
	%% write data;
}

std::string expand_vars(const std::string &s, const Environment &env) {
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
