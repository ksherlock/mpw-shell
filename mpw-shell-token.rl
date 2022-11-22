#include <string>
#include <vector>
#include <stdio.h>

#include "mpw-shell.h"
#include "error.h"

%%{
	machine  tokenizer;
	alphtype unsigned char;


	escape = 0xb6;
	ws = [ \t\n\r];

	action push_token {
		if (!scratch.empty()) {
			tokens.emplace_back(std::move(scratch));
			scratch.clear();
		}
	}

	action push {
		scratch.push_back(fc);
	}

	action push_string {
		scratch.append(ts, te);
	}

	schar = [^'] ;
	sstring = ['] schar** ['] $err{ throw sstring_error(); } ;

	escape_seq = escape any ;

	# double-quoted string.
	dchar = escape_seq | (any - escape - ["]);
	dstring = ["] dchar** ["] $err{ throw dstring_error(); } ;

	# search-forward string
	# fschar = escape_seq | (any - escape - [/]);
	fchar = [^/];
	fstring = [/] fchar** [/] $err{ throw fsstring_error(); } ;

	# search-backward string
	# bschar = escape_seq | (any - escape - [\\]);
	bchar = [^\\];
	bstring = [\\] bchar** [\\] $err{ throw bsstring_error(); } ;

	action eval { eval }

	# > == start state (single char tokens or common prefix)
	# % == final state (multi char tokens w/ unique prefix)
	# $ == all states
	char = any - ['"/\\];
	main := |*
			ws+  >push_token;
			'>>' %push_token => { tokens.emplace_back(">>", '>>'); };
			'>'  %push_token => { tokens.emplace_back(">", '>'); };

			'<'  %push_token => { tokens.emplace_back("<", '<'); };

			# macroman ∑, ∑∑
			0xb7 0xb7 %push_token => { tokens.emplace_back("\xb7\xb7", 0xb7b7); };
			0xb7      %push_token => { tokens.emplace_back("\xb7", 0xb7); };

			# macroman ≥,  ≥≥ 
			0xb3 0xb3 %push_token => { tokens.emplace_back("\xb3\xb3", 0xb3b3); };
			0xb3      %push_token => { tokens.emplace_back("\xb3", 0xb3); };

			# eval-only.

			'||' when eval
				 %push_token => { tokens.emplace_back("||", '||'); };
			'|'  when eval
				 %push_token => { tokens.emplace_back("|", '|'); };

			'&&' when eval
				 %push_token => { tokens.emplace_back("&&", '&&'); };


			'('  when eval
				 %push_token => { tokens.emplace_back("(", '('); };

			')'  when eval
				 %push_token => { tokens.emplace_back(")", ')'); };


			'<<' when eval
				%push_token => { tokens.emplace_back("<<", '<<'); };

			'<=' when eval
				%push_token => { tokens.emplace_back("<=", '<='); };

			'>=' when eval
				%push_token => { tokens.emplace_back(">=", '>='); };

			'==' when eval
				 %push_token => { tokens.emplace_back("==", '=='); };

			'!=' when eval
				 %push_token => { tokens.emplace_back("!=", '!='); };

			'&'  when eval
				 %push_token => { tokens.emplace_back("&", '&'); };

			'+'  when eval
				 >push_token => { tokens.emplace_back("+", '+'); };

			'*'  when eval
				 %push_token => { tokens.emplace_back("*", '*'); };

			'%'  when eval
				 %push_token => { tokens.emplace_back("%", '%'); };


			'-'  when eval
				 %push_token => { tokens.emplace_back("-", '-'); };

			'!'  when eval
				 %push_token => { tokens.emplace_back("!", '!'); };

			'^'  when eval
				 %push_token => { tokens.emplace_back("^", '^'); };

			'~'  when eval
				 %push_token => { tokens.emplace_back("~", '~'); };


			'=' when eval
				 %push_token => { tokens.emplace_back("=", '='); };

			'+=' when eval
				 %push_token => { tokens.emplace_back("+=", '+='); };

			'-=' when eval
				 %push_token => { tokens.emplace_back("-=", '-='); };

			'=~' when eval
				 %push_token => { tokens.emplace_back("=~", '=~'); };

			'!~' when eval
				 %push_token => { tokens.emplace_back("!~", '!~'); };

			sstring => push_string;
			dstring => push_string;
			fstring => push_string;
			bstring => push_string;
			escape_seq => push_string;

			char => push;
		*|
		;
}%%



void replace_eval_token(token &t) {

%%{
	
	machine eval_keywords;

	main := 
		  'and'i %{ t.type = '&&'; } 
		| 'or'i  %{ t.type = '||'; }
		| 'not'i %{ t.type = '!'; }
		| 'div'i %{ t.type = '/'; }
		| 'mod'i %{ t.type = '%'; }
		;
}%%


	%%machine eval_keywords;
	%%write data;
	

	const char *p = t.string.data();
	const char *pe = t.string.data() + t.string.size();
	const char *eof = pe;
	int cs;

	%%write init;

	%%write exec;
}


void unquote(token &t) {

	if (t.string.find_first_of("'\"\xb6", 0, 3) == t.string.npos) return;

	int cs;
	const unsigned char *p = (const unsigned char *)t.string.data();
	const unsigned char *pe = p + t.string.length();
	const unsigned char *eof = pe;

	std::string scratch;
	scratch.reserve(t.string.length());
%%{
	
	machine unquote;
	alphtype unsigned char;

	action push { scratch.push_back(fc); }
	escape = 0xb6;
	char = any - escape - ['"/\\];

	schar = [^'] $push;
	sstring = ['] schar** ['];

	# // and \\ strings retain the delimiter.
	fchar = [^/];
	fstring = ([/] fchar** [/]) $push;

	bchar = [^\\];
	bstring = ([\\] bchar** [\\]) $push;


	ecode = 
		  'f' ${ scratch.push_back('\f'); }
		| 'n' ${ scratch.push_back('\n'); }
		| 't' ${ scratch.push_back('\t'); }
		| [^fnt] ${ scratch.push_back(fc); }
		;

	escape_seq = escape $err{ scratch.push_back(escape); } ecode;

	dchar = escape ecode | (any - escape - ["]) $push;
	dstring = ["] dchar** ["];

	main := (
		  escape_seq
		| sstring
		| fstring
		| bstring
		| dstring
		| char $push
	)**;

	write data;
	write init;
	write exec;
}%%

	t.string = std::move(scratch);
}


std::vector<token> tokenize(std::string &s, bool eval)
{
	std::vector<token> tokens;
	std::string scratch;


	%%machine tokenizer;
	%% write data;

	int cs, act;
	unsigned const char *p = (const unsigned char *)s.data();
	unsigned const char *pe = (const unsigned char *)s.data() + s.size();
	unsigned const char *eof = pe;

	unsigned const char *ts, *te;

	%%write init;

	%%write exec;

	if (!scratch.empty()) {
		tokens.emplace_back(std::move(scratch));
		scratch.clear();
	}

	// re-build s.
	s.clear();
	for (const token &t : tokens) {
		s.append(t.string);
		s.push_back(' ');
	}
	if (!s.empty()) s.pop_back();

	for (token &t : tokens) {
		if (t.type == token::text) unquote(t);
	}

	// alternate operator tokens for eval
	if (eval) {

		for (token & t : tokens) {
			if (t.type == token::text) replace_eval_token(t);

		}
	}

	return tokens;
}
