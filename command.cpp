#include "command.h"
#include "phase2-parser.h"
#include <stdexcept>

command::~command()
{}


int simple_command::execute() {
	// echo
	fprintf(stderr, "  %s\n", text.c_str());
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

	// echo!
	fprintf(stderr, "  %s ... %s\n",
		type == BEGIN ? "begin" : "(",
		end.c_str()
	);

	int rv = vector_command::execute();
	fprintf(stderr, "  %s\n", type == BEGIN ? "end" : ")");
	return rv;
}

int if_command::execute() {

	int rv = 0;
	bool ok = false;


	// todo -- parse end for redirection.
	for (auto &c : clauses) {
		if (c->type == IF) {// special.
			fprintf(stderr, "  %s ... %s\n",
				c->clause.c_str(), end.c_str());
		}
		else {
			fprintf(stderr, "  %s\n", c->clause.c_str());
		}

		if (ok) continue;
		ok = c->evaluate();
		if (!ok) continue;
		rv = c->execute();
	}

	fprintf(stderr, "  end\n");
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
