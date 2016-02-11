/*
 * phase1 -- strip comments and merge multi-lines.
 *
 */


#include "phase1.h"
#include <stdexcept>
#include <stdint.h>

const unsigned char escape = 0xb6;

/*
 * from experimentation, mpw splits on ; after variable expansion; 
 * this splits before.  something stupid like:
 * set q '"'; echo {q} ; "
 * will not be handled correctly.  oh well.
 * (should probably just drop that and we can then combine tokenizing w/ 
 * variable expansion)
 */
%%{
	machine main;
	alphtype unsigned char;


	escape = 0xb6;
	ws = [ \t];
	nl = ('\n' | '\r');

	action add_line {
		/* strip trailing ws */
		while (!scratch.empty() && isspace(scratch.back())) scratch.pop_back();

		if (!scratch.empty()) { 
			std::string tmp = std::move(scratch);
			scratch.clear();
			if (pipe_to) pipe_to(std::move(tmp));
		}
		fgoto main;
	}

	action push_back {
		scratch.push_back(fc);
	}

	action push_back_escape {
		scratch.push_back(escape);
		scratch.push_back(fc);
	}

	comment = '#' (any-nl)*;

	escape_seq =
		escape
		(
			nl ${ /* esc newline */ line++; }
			|
			(any-nl) $push_back_escape
		)
	;


	# single-quoted string.  only escape \n is special.
	# handling is so stupid I'm not going to support it.

	sstring = 
		['] $push_back
		( (any-nl-[']) $push_back )*
		['] $push_back
		$err{
			throw std::runtime_error("### MPW Shell - 's must occur in pairs.");
		}
	;

	# todo -- {{variables}}
	# same quoting logic as ' string
	vstring = 
		'{' $push_back
		( (any-nl-'}') $push_back )*
		'}' $push_back
		$err{
			throw std::runtime_error("### MPW Shell - {s must occur in pairs.");
		}
	;


	# double-quoted string.  
	# escape \n is ignored.  others do nothing.
	dstring =
		["] $push_back
		(
			escape_seq
			|
			vstring
			|
			(any-escape-nl-["{]) $push_back
		)* ["] $push_back
		$err{
			throw std::runtime_error("### MPW Shell - \"s must occur in pairs.");
		}
	;

	# gobble up all the white space...
	coalesce_ws =
		ws+
		<:
		''
		%{ if (!scratch.empty() && scratch.back() != ' ') scratch.push_back(' '); }
		;

	line :=
	(
		sstring
		|
		dstring
		|
		vstring
		|
		escape_seq
		|
		coalesce_ws
		|
		(any-escape-nl-ws-[#'"{]) $push_back
	)*
	comment?
	nl ${ line++; } $add_line
	;

	main :=
		# strip leading whitespace.
		ws*
		<: # left guard -- higher priority to ws.
		any ${ fhold; fgoto line; }
	;

}%%



namespace {
	%% write data;	
}

phase1::phase1() {
	%% write init;
}

void phase1::reset() {
	%% write init;
	scratch.clear();
	// line = 1?
}

void phase1::process(const unsigned char *begin, const unsigned char *end, bool final) {
	
	int start_line;

	const unsigned char *p = begin;
	const unsigned char *pe = end;
	const unsigned char *eof = nullptr;

	if (final)
		eof = pe;

	%% write exec;

	if (cs == main_error) {
		throw std::runtime_error("MPW Shell - Lexer error.");
	}

#if 0
	if (cs != main_start && final) {
		// will this happen?
		throw std::runtime_error("MPW Shell - Lexer error.");
	}
#endif
}
