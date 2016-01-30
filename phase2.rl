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

	ws = [ \t];
	
	main := |*
		'||' when not_special => {
				flush();
				parse(PIPE_PIPE, std::string(ts, te));
		};

		'&&' when not_special => {
				flush();
				parse(AMP_AMP, std::string(ts, te));
		};

		'(' when not_special => {
				flush();
				parse(LPAREN, std::string(ts, te));
		};

		# ) may include redirection so start a new token but don't parse it yet.
		')' when not_special => {
				flush();
				scratch.push_back(fc);
				type = RPAREN;
		};

		# todo -- also add in strings and escapes.

		';' => { flush(); parse(SEMI, ";"); };
		ws  => { if (!scratch.empty()) scratch.push_back(fc); };
		any => { scratch.push_back(fc); };
	*|;
}%%


%%{
	machine classify;
	alphtype unsigned char;

	ws = [ \t];

	IF = /if/i;
	ELSE = /else/i;
	END = /end/i;
	BEGIN = /begin/i;
	EVALUATE = /evaluate/i;


	main := |*
		IF %eof{ type = IF; return; };
		IF ws => { type = IF; return; };

		ELSE %eof{ type = ELSE; return; };
		ELSE ws => { type = ELSE; return; };

		ELSE ws+ IF %eof{ type = ELSE_IF; return; };
		ELSE ws+ IF ws => { type = ELSE_IF; return; };

		EVALUATE %eof{ type = EVALUATE; return; };
		EVALUATE ws => { type = EVALUATE; return; };

		END %eof{ type = END; return; };
		END ws => { type = END; return; };

		BEGIN %eof{ type = BEGIN; return; };
		BEGIN ws => { type = BEGIN; return; };

		')' => { type = LPAREN; return; };
	*|;

}%%

namespace {
	%% machine classify;
	%% write data;
	%% machine main;
	%% write data;
}

void phase2::flush() {
	// remove white space...
	while (!scratch.empty() && isspace(scratch.back())) scratch.pop_back();


	if (!scratch.empty()) {
		if (!type) classify();
		parse(type, std::move(scratch));
	}

	type = 0;
	scratch.clear();
}

bool phase2::special() {
	if (!type) classify();

	switch (type) {
	case IF:
	case ELSE:
	case ELSE_IF:
	case EVALUATE:
		return true;
	default:
		return false;
	}
}

void phase2::classify() {
	if (type) return;
	if (scratch.empty()) return;

	int cs;
	int act;
	const unsigned char *p = (const unsigned char *)scratch.data();
	const unsigned char *pe = p + scratch.size();
	const unsigned char *eof = pe;
	const unsigned char *te, *ts;

	type = COMMAND;

	%% machine classify;
	%% write init;
	%% write exec;
}

void phase2::process(const std::string &line) {
	

	int cs;
	int act;
	const unsigned char *p = (const unsigned char *)line.data();
	const unsigned char *pe = p + line.size();
	const unsigned char *eof = pe;
	const unsigned char *te, *ts;

	scratch.clear();
	type = 0;

	%% machine main;
	%% write init;
	%% write exec;

	flush();
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
		for (auto &p : parser->command_queue) {
			if (p) {
				pipe_to(std::move(p));
			}
		}
		parser->command_queue.clear();
	}
	
}

phase2::phase2() {
	parser = std::move(phase2_parser::make());
}

#pragma mark - phase2_parser

void phase2_parser::parse_accept() {
	error = false;
}

void phase2_parser::parse_failure() {
	error = false;
}

void phase2_parser::syntax_error(int yymajor, std::string &yyminor) {
	if (!error)
		fprintf(stderr, "### Parse error near %s\n", yyminor.c_str());

	error = true;
}
