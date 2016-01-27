#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include <stdio.h>
#include <assert.h>

#include "mpw-shell.h"

%%{
	machine classify;
	alphtype unsigned char;

	ws = [ \t];

	IF = /if/i;
	ELSE = /else/i;
	END = /end/i;
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


int classify(const std::string &line) {

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
 * Generates a linked-list of commands. Why? Because it also checks
 * for shell-special syntax (currently if / else /end only) and 
 * adds pointers to make executing them easier.
 *
 */
command_ptr build_command(const std::vector<std::string> &lines) {
	
	std::vector<command_ptr> if_stack;

	command_ptr head;
	command_ptr prev;

	for (const auto &line : lines) {
		if (line.empty()) continue;

		int type = classify(line);
		command_ptr c = std::make_shared<command>(type, line);

		if (!head) head = c;
		if (!prev) prev = c;
		else {
			prev->next = c;
			prev = c;
		}

		// if stack...
		switch (type) {
		case command_if:
			if_stack.push_back(c);
			break;

		case command_else:
		case command_else_if:

			if (if_stack.empty()) {
				throw std::runtime_error("### MPW Shell - Else must be within if ... end.");
			}	

			if_stack.back()->alternate = c;
			if_stack.back() = c;
			break;

		case command_end:
			if (if_stack.empty()) {
				throw std::runtime_error("### MPW Shell - Extra end command.");
			}
			if_stack.back()->alternate = c;
			if_stack.pop_back();
			break;
		}
	}

	if (!if_stack.empty()) {
		throw std::runtime_error("### MPW Shell - Unterminated if command.");
	}

	return head;
}
