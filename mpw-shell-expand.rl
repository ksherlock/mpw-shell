

#include <vector>
#include <string>
#include <unordered_map>

#include <stdio.h>

#include "mpw-shell.h"


%%{
	machine line_parser;
	alphtype unsigned char;

	escape = 0xb6;
	ws = [ \t];
	nl = '\n';

	action push_back {
		line.push_back(fc);
	}
	action push_back_escape {
		line.push_back(escape);
		line.push_back(fc);
	}


	sstring = 
		['] $push_back
		( (any-nl-[']) $push_back )*
		['] $push_back
		$err{
			fprintf(stderr, "### MPW Shell - 's must occur in pairs.\n");
		}
	;

	# same quoting logic as ' string
	vstring = 
		'{'
		( (any-nl-'}') ${var.push_back(fc); } )*
		'}'
		${
			if (!var.empty()) {

				// flag to pass through vs "" ?
				auto iter = env.find(var);
				if (iter == env.end()) {
					if (env.passthrough()) {
						line.push_back('{');
						line.append(var);
						line.push_back('}');
					}
				}
				else {
					line.append((std::string)iter->second);
				}
			}
			var.clear();
		}
		$err{
			fprintf(stderr, "### MPW Shell - {s must occur in pairs.\n");
		}
	;


	# double-quoted string.  
	# escape \n is ignored.  others do nothing.
	dstring =
		["] $push_back
		(
			escape (
				nl ${ /* esc newline */ }
				|
				(any-nl) $push_back_escape
			)
			|
			vstring
			|
			(any-escape-nl-["{]) $push_back
		)* ["] $push_back
		$err{
			fprintf(stderr, "### MPW Shell - \"s must occur in pairs.\n");
		}
	;


	main :=
		(
			sstring
			|
			dstring
			|
			vstring
			|
			escape any $push_back_escape
			|
			(any-['"{]) $push_back
		)*
		;




}%%



%% write data;


/*
 * has to be done separately since you can do dumb stuff like:
 * set q '"' ; echo {q} dsfsdf"
 */

std::string expand_vars(const std::string &s, const Environment &env) {
	
	if (s.find('{') == s.npos) return s;
	std::string var;
	std::string line;

	int cs;
	const unsigned char *p = (const unsigned char *)s.data();
	const unsigned char *pe = (const unsigned char *)s.data() + s.size();
	const unsigned char *eof = pe;

	%%write init;

	%%write exec;

	return line;
}

