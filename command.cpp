#include "command.h"
#include "phase2-parser.h"
#include <stdexcept>

command::~command()
{}


int simple_command::execute() {
	return 0;
}

int or_command::execute() {
	/*
	for (const auto &c : children) {
		if (!c) throw std::runtime_error("corruption in || command.");
	}
	*/

	int rv = 0;
	for (auto &c : children) {
		rv = c->execute();
		if (rv == 0) return rv;
	}
	return rv;
}

int and_command::execute() {

	int rv = 0;
	for (auto &c : children) {
		rv = c->execute();
		if (rv != 0) return rv;
	}
	return rv;
}


int vector_command::execute() {

	int rv = 0;
	for (auto &c : children) {

		rv = c->execute();
		// todo -- env.exit
	}
	return rv;
}

int begin_command::execute() {
	// todo -- parse end for redirection.

	int rv = vector_command::execute();

	return rv;
}

int if_command::execute() {

	int rv = 0;
	bool ok = false;
	// todo -- parse end for redirection.
	for(auto &c : clauses) {
		if (ok) continue;
		ok = c->evaluate();
		if (!ok) continue;
		rv = c->execute();
	}
	return rv;
}

/*
int if_else_clause::execute() {
	return 0;
}
*/

bool if_else_clause::evaluate() {
	return false;
}
