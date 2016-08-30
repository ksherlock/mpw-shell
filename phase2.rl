/*
 * phase2 -- parse a line into major control structures (begin/end/if/etc)
 * input is a full line -- comments have been removed, escape-nl handled, trailing newline stripped. 
 *
 */

#include "phase2.h"
#include "phase3.h"

%%{
	machine main;
	alphtype unsigned char;

	action not_special { !special() }

	action parse_ws {
		if (scratch.empty()) fgoto main;
	}
	action parse_semi {
		flush();
		parse(SEMI, ";");
		fgoto main;
	}


	action parse_amp_amp {
		if (!special()) {
			scratch.pop_back();
			flush();
			parse(AMP_AMP, "&&");
			fgoto main;
		}
	}

	action parse_pipe_pipe {
		if (!special()) {
			scratch.pop_back();
			flush();
			parse(PIPE_PIPE, "||");
			fgoto main;
		}
	}

	action parse_pipe_any {
		if (!special()) {
			scratch.pop_back();
			flush();
			parse(PIPE, "|");
		}
		fhold;
		fgoto main;
	}

	action parse_pipe_eof {

		if (!special()) {
			scratch.pop_back();
			flush();
			parse(PIPE, "|");
		}
	}

	action parse_lparen {
		if (scratch.empty()) {
			parse(LPAREN, "(");
			fgoto main;
		}
		pcount++;
	}

	action parse_rparen {
		if (pcount <= 0) {
			flush();
			parse(RPAREN, ")");
			fgoto main;
		}
		--pcount;
	}

	escape = 0xb6;
	ws = [ \t];


	escape_seq = escape any ;

	schar = [^'];
	sstring = ['] schar** ['] ;

	vchar = [^}];
	vstring = [{] vchar** [}] ;

	# double-quoted string.
	dchar = escape_seq | (any - escape - ["]) ; 
	dstring = ["] dchar** ["];


	echar = escape_seq | (any - escape - [`]) ; 
	estring1 = '`' echar** '`';
	estring2 = '``' echar** '``';
	estring = estring1 | estring2 ;

	# default action is to push character into scratch.
	# fgoto main inhibits.
	main := (
		  ws $parse_ws
		| ';' $parse_semi
		| '(' $parse_lparen
		| ')' $parse_rparen
		| '|' <eof(parse_pipe_eof)
		| '|' [^|] $parse_pipe_any
		| '|' '|' $parse_pipe_pipe
		| '&' '&' $parse_amp_amp
		| escape_seq
		| sstring
		| dstring
		| vstring
		| estring
		| any
	)** ${ scratch.push_back(fc); };

}%%


%%{
	machine argv0;	
	alphtype unsigned char;

	action push { argv0.push_back(tolower(fc)); }
	action break { fbreak; }

	escape = 0xb6;
	ws = [ \t];


	# ` and { not supported here.


	# hmmm ... only push the converted char  - escape n = \n, for example.
	esc_seq =
		escape (
			'f' ${argv0.push_back('\f'); } |
			'n' ${argv0.push_back('\n'); } |
			't' ${argv0.push_back('\t'); } |
			[^fnt] $push
		);

	schar = [^'] $push;
	sstring = ['] schar** ['];

	dchar = esc_seq | (any-escape-["]) $push;
	dstring = ["] dchar** ["];

	# mpw doesn't handle quotes at this point,
	# so simplify and stop if we see anything invalid.
	main := (
		  ws $break
		| [|<>] $break
		| 0xb7 $break
		| 0xb3 $break
		| [^a-zA-Z] ${ return COMMAND; }
		| any $push
	)**;

}%%


int phase2::classify() {

	%%machine argv0;
	%%write data;
	
	if (type) return type;
	std::string argv0;

	const unsigned char *p = (const unsigned char *)scratch.data();
	const unsigned char *pe = p + scratch.size();
	int cs;

	type = COMMAND;

	%%write init;
	%%write exec;

//	fprintf(stderr, "%s -> %s\n", scratch.c_str(), argv0.c_str());
#undef _
#define _(a,b) if (argv0 == a) { type = b; return type; }

	// expand aliases?

	_("begin", BEGIN)
	_("break", BREAK)
	_("continue", CONTINUE)
	_("else", ELSE)
	_("end", END)
	_("evaluate", EVALUATE)
	_("exit", EXIT)
	_("for", FOR)
	_("if", IF)
	_("loop", LOOP)

#undef _
	return type;
}


namespace {
	%% machine argv0;
	%% write data;

	%% machine main;
	%% write data;
}

void phase2::flush() {
	//fprintf(stderr, "flush: %s\n", scratch.c_str());
	// remove white space...
	while (!scratch.empty() && isspace(scratch.back())) scratch.pop_back();


	if (!scratch.empty()) parse(classify(), std::move(scratch));

	type = 0;
	pcount = 0;
	scratch.clear();
}

/* slightly wrong since whitespace is needed for it to be special. */
bool phase2::special() {

	switch (classify()) {
	case IF:
	case ELSE:
	case ELSE_IF:
	case EVALUATE:
	case BREAK:
	case CONTINUE:
	case EXIT:
		return true;
	default:
		return false;
	}
}

void phase2::parse(int type, std::string &&s) {
	if (_then) _then(type, std::move(s));
}

void phase2::parse(std::string &&line) {
	
	//fprintf(stderr, "-> %s\n", line.c_str());

	int cs;
	const unsigned char *p = (const unsigned char *)line.data();
	const unsigned char *pe = p + line.size();
	const unsigned char *eof = pe;

	scratch.clear();
	type = 0;
	pcount = 0; // parenthesis balancing within command only.

	%% machine main;
	%% write init;
	%% write exec;

	flush();
	if (_then) {
		_then(NL, "");
		_then(NL, "");
	}
}

void phase2::finish() {
}

void phase2::reset() {

	type = 0;
	pcount = 0;
	scratch.clear();
}

