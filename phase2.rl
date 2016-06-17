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


	escape = 0xb6;
	ws = [ \t];


	escape_seq =
		escape any
	;

	sstring = 
		[']
		( (any-[']) )*
		[']
		$err{
			throw std::runtime_error("### MPW Shell - 's must occur in pairs.");
		}
	;

	vstring = 
		[{]
		( (any-[}]) )*
		[}]
		$err{
			throw std::runtime_error("### MPW Shell - {s must occur in pairs.");
		}
	;

	# double-quoted string.
	dstring =
		["]
		(
			escape_seq
			|
			(any-escape-["])
		)*
		["]
		$err{
			throw std::runtime_error("### MPW Shell - \"s must occur in pairs.");
		}
	;



	main := |*

		'||' when not_special => {
				flush();
				parse(PIPE_PIPE, std::string(ts, te));
		};

		'&&' when not_special => {
				flush();
				parse(AMP_AMP, std::string(ts, te));
		};


		# ( evaluate (1+2) ) is lparen, eval, rparen.
		# need to balance parens here and terminate a special token when it goes negative.


		'(' => {
			if (special()) {
				pcount++;
				scratch.push_back(fc);
			} else {
				flush();
				parse(LPAREN, std::string(ts, te));
			}
		};

		')' => {
			if (special() && pcount-- > 0) scratch.push_back(fc);
			else {			
				flush();
				scratch.push_back(fc);
				type = RPAREN;
			}
		};


		';' => { flush(); parse(SEMI, ";"); };

		ws  => { if (!scratch.empty()) scratch.push_back(fc); };

		sstring => { scratch.append(ts, te); };
		dstring => { scratch.append(ts, te); };
		vstring => { scratch.append(ts, te); };
		escape_seq => { scratch.append(ts, te); };


		(any-escape-['"{]) => { scratch.push_back(fc); };
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
	LOOP = /loop/i;
	FOR = /for/i;
	BREAK = /break/i;
	CONTINUE = /continue/i;


	main := |*
		IF %eof{ type = IF; return; };
		IF ws => { type = IF; return; };

		ELSE %eof{ type = ELSE; return; };
		ELSE ws => { type = ELSE; return; };

		#ELSE ws+ IF %eof{ type = ELSE_IF; return; };
		#ELSE ws+ IF ws => { type = ELSE_IF; return; };

		EVALUATE %eof{ type = EVALUATE; return; };
		EVALUATE ws => { type = EVALUATE; return; };

		END %eof{ type = END; return; };
		END ws => { type = END; return; };

		BEGIN %eof{ type = BEGIN; return; };
		BEGIN ws => { type = BEGIN; return; };

		LOOP %eof{ type = LOOP; return; };
		LOOP ws => { type = LOOP; return; };

		FOR %eof{ type = FOR; return; };
		FOR ws => { type = FOR; return; };

		BREAK %eof{ type = BREAK; return; };
		BREAK ws => { type = BREAK; return; };

		CONTINUE %eof{ type = CONTINUE; return; };
		CONTINUE ws => { type = CONTINUE; return; };


		'(' => { type = LPAREN; return; };
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

/* slightly wrong since whitespace is needed for it to be special. */
bool phase2::special() {
	if (!type) classify();

	switch (type) {
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
	
	if (line.empty()) { finish(); return; }

	int pcount = 0; // special form parens cannot cross lines.

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

