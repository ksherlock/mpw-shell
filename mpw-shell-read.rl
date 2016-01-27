#include "mpw-shell.h"

#include <unistd.h>
#include <fcntl.h>
#include <algorithm>

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
		IF %eof{ return command_if; };
		IF ws => {return command_if; };

		ELSE %eof{ return command_else;};
		ELSE ws => { return command_else; };

		ELSE ws+ IF %eof{ return command_else_if; };
		ELSE ws+ IF ws => {return command_else_if; };

		END %eof{ return command_end; };
		END ws => {return command_end; };

		EVALUATE %eof{ return command_evaluate; };
		EVALUATE ws => {return command_evaluate; };


	*|;

}%%


static int classify(const std::string &line) {

	%%machine classify;
	%% write data;

	int cs;
	int act;

	const unsigned char *p = (const unsigned char *)line.data();
	const unsigned char *pe = (const unsigned char *)line.data() + line.size();
	const unsigned char *eof = pe;
	const unsigned char *te, *ts;

	%%write init;

	%%write exec;

	return 0;	
}


/*
 * this state machine splits input into lines.  
 * only new-line escapes are removed.
 * "", '', and {} are also matched.
 *
 */

/*
 * from experimentation, mpw splits on ; after variable expansion; 
 * this splits before.  something stupid like:
 * set q '"'; echo {q} ; "
 * will not be handled correctly.  oh well.
 * (should probably just drop that and we can then combine tokenizing w/ 
 * variable expansion)
 */
%%{
	machine line_parser;
	alphtype unsigned char;


	escape = 0xb6;
	ws = [ \t];
	nl = ('\n' | '\r');

	action add_line {
		/* strip trailing ws */
		while (!scratch.empty() && isspace(scratch.back())) scratch.pop_back();
		if (!scratch.empty()) { 
			command_ptr cmd = std::make_shared<command>(std::move(scratch));
			cmd->line = start_line;
			start_line = line;
			program.emplace_back(std::move(cmd));
		}
		scratch.clear();
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

	# this is a mess ...
	coalesce_ws =
		ws
		(
			ws
			|
			escape nl ${ line++; }
		)*
		<:
		any ${ scratch.push_back(' '); fhold; }
	;

	line :=
	(
		sstring
		|
		dstring
		|
		vstring
		|
		[;] $add_line
		|
		escape_seq
		|
		coalesce_ws
		|
		(any-escape-nl-ws-[;#'"{]) $push_back
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






class line_parser {

	public:

	void process(const void *data, size_t size) {
		process((const unsigned char *)data, size, false);
	}

	command_ptr finish() {
		process((const unsigned char *)"\n\n", 2, true);
		return build_program();
	}	

	line_parser();

	private:

	%% machine line_parser;
	%% write data;


	std::vector<command_ptr> program;
	std::string scratch;
	int line = 1;
	int cs;

	command_ptr build_program();
	void process(const unsigned char *data, size_t size, bool final);
};

line_parser::line_parser() {
	%% machine line_parser;
	%% write init;
}

void line_parser::process(const unsigned char *data, size_t size, bool final) {
	
	int start_line;

	const unsigned char *p = data;
	const unsigned char *pe = data + size;
	const unsigned char *eof = nullptr;

	if (final)
		eof = pe;

	start_line = line;
	%% machine line_parser;
	%% write exec;

	if (cs == line_parser_error) {
		throw std::runtime_error("MPW Shell - Lexer error.");

	}

	if (cs != line_parser_start && final) {
		// will this happen?
		throw std::runtime_error("MPW Shell - Lexer error.");
	}
}


/*
 * Generates a linked-list of commands. Why? Because it also checks
 * for shell-special syntax (currently if / else /end only) and 
 * adds pointers to make executing them easier.
 *
 */

// todo -- use recursive descent parser, support begin/end, (), ||, &&, etc.
command_ptr line_parser::build_program() {


	std::vector<command_ptr> if_stack;

	command_ptr head;
	command_ptr ptr;

	if (program.empty()) return head;

	std::reverse(program.begin(), program.end());

	head = program.back();

	while (!program.empty()) {

		if (ptr) ptr->next = program.back();

		ptr = std::move(program.back());
		program.pop_back();

		int type = ptr->type = classify(ptr->string);

		ptr->level = if_stack.size();

		// if stack...
		switch (type) {
		default:
			break;

		case command_if:
			if_stack.push_back(ptr);
			break;

		case command_else:
		case command_else_if:

			if (if_stack.empty()) {
				throw std::runtime_error("### MPW Shell - Else must be within if ... end.");
			}	

			ptr->level--;
			if_stack.back()->alternate = ptr;
			if_stack.back() = ptr;
			break;

		case command_end:
			if (if_stack.empty()) {
				throw std::runtime_error("### MPW Shell - Extra end command.");
			}

			ptr->level--;
			if_stack.back()->alternate = ptr;
			if_stack.pop_back();
			break;
		}
	}

	if (!if_stack.empty()) {
		throw std::runtime_error("### MPW Shell - Unterminated if command.");
	}

	return head;
}


command_ptr read_fd(int fd) {
	unsigned char buffer[1024];

	line_parser p;

	for(;;) {
		ssize_t s = read(fd, buffer, sizeof(buffer));
		if (s < 0) {
			throw std::runtime_error("MPW Shell - Read error.");
		}
		p.process(buffer, s);
	}
	return p.finish();
}

command_ptr read_file(const std::string &name) {
	int fd;
	fd = open(name.c_str(), O_RDONLY);
	if (fd < 0) {
		throw std::runtime_error("MPW Shell -  Unable to open file " + name + ".");
	}

	auto tmp = read_fd(fd);
	close(fd);
	return tmp;
}

command_ptr read_string(const std::string &s) {
	line_parser p;

	p.process(s.data(), s.size());
	return p.finish();
}
