#include <string>
#include <vector>
#include <stdio.h>

#include "mpw-shell.h"
#include "error.h"

%%{
	machine  tokenizer;
	alphtype unsigned char;


	escape = 0xb6;
	ws = [ \t];
	nl = '\n' | '\r';

	action push_token {
		if (!scratch.empty() || quoted) {
			tokens.emplace_back(std::move(scratch));
			scratch.clear();
			quoted = false;
		}
	}

	action push_back {
		scratch.push_back(fc);
	}

	schar = [^'] $push_back;
	sstring = 
		['] schar** [']
		${ quoted = true; }
		$err{ throw sstring_error(); }
	;

	escape_seq =
		escape
		(
			  'f' ${scratch.push_back('\f'); }
			| 'n' ${scratch.push_back('\n'); /* \r ? */ }
			| 't' ${scratch.push_back('\t'); }
			| [^fnt] $push_back
		)
	;

	# double-quoted string.
	dchar = escape_seq | (any - escape - ["]) $push_back;
	dstring =
		["] dchar** ["]
		${ quoted = true; }
		$err{ throw dstring_error(); }
	;


	action eval { eval }

	# > == start state (single char tokens or common prefix)
	# % == final state (multi char tokens w/ unique prefix)
	# $ == all states

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
				 %push_token => { tokens.emplace_back("+", '-'); };

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


			sstring ;
			dstring ;
			escape_seq;

			(any-escape-['"]) => push_back;
		*|
		;
}%%



inline void replace_eval_token(token &t) {

%%{
	
	machine eval_keywords;

	main := 
		/and/i %{ t.type = '&&'; } 
		|
		/or/i %{ t.type = '||'; }
		|
		/not/i %{ t.type = '!'; }
		|
		/div/i %{ t.type = '/'; }
		|
		/mod/i %{ t.type = '%'; }
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
std::vector<token> tokenize(const std::string &s, bool eval)
{
	std::vector<token> tokens;
	std::string scratch;
	bool quoted = false; // found a quote character ("" creates a token)


	%%machine tokenizer;
	%% write data;

	int cs, act;
	unsigned const char *p = (const unsigned char *)s.data();
	unsigned const char *pe = (const unsigned char *)s.data() + s.size();
	unsigned const char *eof = pe;

	unsigned const char *ts, *te;

	%%write init;

	%%write exec;

	if (cs == tokenizer_error) {
		throw std::runtime_error("MPW Shell - Lexer error.");
	}

	if (!scratch.empty() || quoted) {
		tokens.emplace_back(std::move(scratch));
		scratch.clear();
	}

	// alternate operator tokens for eval
	if (eval) {

		for (token & t : tokens) {
			if (t.type == token::text) replace_eval_token(t);

		}
	}

	return tokens;
}
