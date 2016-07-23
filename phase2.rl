/*
 * phase2 -- parse a line into major control structures (begin/end/if/etc)
 * input is a full line -- comments have been removed, escape-nl handled, trailing newline stripped. 
 *
 */

#include "phase2-parser.h"
#include "phase2.h"
#include "command.h"

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

	main := (
		  ws ${ fbreak; }
		| [{`] ${ argv0.clear(); fbreak; }
		| sstring
		| dstring
		| esc_seq
		| (any-escape-['"]) $push
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


	if (!scratch.empty()) {
		parse(classify(), std::move(scratch));
	}

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
		return true;
	default:
		return false;
	}
}

void phase2::process(const std::string &line) {
	
	//fprintf(stderr, "-> %s\n", line.c_str());

	// still needed?
	if (line.empty()) { finish(); return; }


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
	// 2 NLs to make the stack reduce.  harmless if in a multi-line constuct.
	parse(NL, "");
	parse(NL, "");

	exec();
}

void phase2::finish() {
	parse(0, "");
	exec();
}

void phase2::parse(int token, std::string &&s) {
	if (parser) parser->parse(token, std::move(s));
}

void phase2::exec() {

	if (pipe_to && parser) {
		command_ptr_vector tmp;
		for (auto &p : parser->command_queue) {
			if (p) tmp.emplace_back(std::move(p));
		}
		parser->command_queue.clear();

		if (!tmp.empty()) pipe_to(std::move(tmp));
	}
	
}

phase2::phase2() {
	parser = phase2_parser::make();
	//parser->trace(stdout, " ] ");

}

void phase2::abort() {
	parser = nullptr;
	parser = phase2_parser::make();
}

#pragma mark - phase2_parser

void phase2_parser::parse_accept() {
	error = false;
}

void phase2_parser::parse_failure() {
	error = false;
}

void phase2_parser::syntax_error(int yymajor, std::string &yyminor) {
/*
	switch (yymajor) {
	case END:
		fprintf(stderr, "### MPW Shell - Extra END command.\n");
		break;

	case RPAREN:
		fprintf(stderr, "### MPW Shell - Extra ) command.\n");
		break;

	case ELSE:
	case ELSE_IF:
		fprintf(stderr, "### MPW Shell - ELSE must be within IF ... END.\n");
		break;

	default:
		fprintf(stderr, "### Parse error near %s\n", yyminor.c_str());
		break;
	}
*/

	
	fprintf(stderr, "### MPW Shell - Parse error near %s\n", yymajor ? yyminor.c_str() : "EOF");
	error = true;
}

