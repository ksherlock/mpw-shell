#include "command.h"
#include "phase2-parser.h"
#include "environment.h"
#include "fdset.h"
#include "builtins.h"
#include "mpw-shell.h"

#include <stdexcept>
#include <unordered_map>
#include <cctype>
#include <cerrno>
#include <cstdlib>


#include <unistd.h>
#include <sys/wait.h>
#include <sysexits.h>

namespace {

	std::string &lowercase(std::string &s) {
		std::transform(s.begin(), s.end(), s.begin(), [](char c){ return std::tolower(c); });
		return s;
	}

	std::unordered_map<std::string, int (*)(Environment &, const std::vector<std::string> &, const fdmask &)> builtins = {
		{"directory", builtin_directory},
		{"echo", builtin_echo},
		{"parameters", builtin_parameters},
		{"quote", builtin_quote},
		{"set", builtin_set},
		{"unset", builtin_unset},
		{"export", builtin_export},
		{"unexport", builtin_unexport},
	};




	int execute_external(const Environment &env, const std::vector<std::string> &argv, const fdmask &fds) {

		std::vector<char *> cargv;
		cargv.reserve(argv.size() + 3);

		int status;
		int pid;

		cargv.push_back((char *)"mpw");
		//cargv.push_back((char *)"--shell");

		unsigned offset = cargv.size();


		std::transform(argv.begin(), argv.end(), std::back_inserter(cargv),
			[](const std::string &s) { return strdup(s.c_str()); }
		);

		cargv.push_back(nullptr);


		pid = fork();
		if (pid < 0) {
			perror("fork: ");
			exit(EX_OSERR);
		}


		if (pid == 0) {

			// also export environment...

			// handle any indirection...
			fds.dup();

			execvp(cargv.front(), cargv.data());
			perror("execvp: ");
			exit(EX_OSERR);
		}

		std::for_each(cargv.begin()+offset, cargv.end(), free);

		for(;;) {
			int status;
			pid_t ok;
			ok = waitpid(pid, &status, 0);
			if (ok < 0) {
				if (errno == EINTR) continue;
				perror("waitpid:");
				exit(EX_OSERR);
			}

			if (WIFEXITED(status)) return WEXITSTATUS(status);
			if (WIFSIGNALED(status)) return -9; // command-. user abort exits via -9.

			fprintf(stderr, "waitpid - unexpected result\n");
			exit(EX_OSERR);
		}

	}


}



//std::string expand_vars(const std::string &s, const class Environment &);

command::~command()
{}

/*
 * todo -- all errors should be handled the same, regardless of source. (throw, parse, etc).
 * echo and error should respect the active fdmask.
 */

int simple_command::execute(Environment &env, const fdmask &fds) {
	std::string s = expand_vars(text, env);

	if (env.echo()) fprintf(stderr, "  %s\n", s.c_str()); 

	process p;
	try {
		auto tokens = tokenize(s, false);
		parse_tokens(std::move(tokens), p);
	} catch(std::exception &e) {
		fprintf(stderr, "%s", e.what());
		return env.status(-4);
	}

	if (p.arguments.empty()) return 0;

	fdmask newfds = p.fds | fds;

	std::string name = p.arguments.front();
	lowercase(name);

	auto iter = builtins.find(name);
	if (iter != builtins.end()) {
		int status = iter->second(env, p.arguments, newfds);
		return env.status(status);
	}

	if (env.test()) return env.status(0);
	if (env.startup()) {
		fprintf(stderr, "### MPW Shell - startup file may not contain external commands.\n");
		return env.status(0);
	}

	int status = execute_external(env, p.arguments, newfds);

	return env.status(status);
}

int evaluate_command::execute(Environment &env, const fdmask &fds) {

	std::string s = expand_vars(text, env);

	if (env.echo()) fprintf(stderr, "  %s\n", s.c_str());

	auto tokens = tokenize(s, true);
	if (tokens.empty()) return 0;

	int status =  builtin_evaluate(env, std::move(tokens), fds);

	return env.status(status);
}


int or_command::execute(Environment &e, const fdmask &fds) {

	int rv = 0;
	bool pv = e.and_or(true);

	for (auto &c : children) {
		rv = c->execute(e, fds);
		if (rv == 0) return rv;
	}
	e.and_or(pv);

	return e.status(rv);
}

int and_command::execute(Environment &e, const fdmask &fds) {

	int rv = 0;
	bool pv = e.and_or(true);

	for (auto &c : children) {
		rv = c->execute(e, fds);
		if (rv != 0) return rv;
	}
	e.and_or(pv);

	return rv;
}


int vector_command::execute(Environment &e, const fdmask &fds) {

	int rv = 0;
	for (auto &c : children) {

		rv = c->execute(e, fds);
		if (e.exit()) break;
	}
	return e.status(rv);
}

int error_command::execute(Environment &e, const fdmask &fds) {
	std::string s = expand_vars(text, e);

	if (e.echo()) fprintf(stderr, "  %s\n", s.c_str());

	switch(type) {
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
	}


	return e.status(-3);
}

namespace {

	int check_ends(const std::string &s, fdmask &fds) {

		// MPW ignores any END tokens other than redirection.
		process p;
		try {
			auto tokens = tokenize(s, false);
			parse_tokens(std::move(tokens), p);
		}
		catch (std::exception &e) {
			fprintf(stderr, "%s\n", e.what());
			return -4;
		}
		fds = p.fds.to_mask();
		return 0;
	}
}

int begin_command::execute(Environment &env, const fdmask &fds) {
	// todo -- parse end for redirection.

	std::string b = expand_vars(begin, env);
	std::string e = expand_vars(end, env);

	// echo!
	if (env.echo()) fprintf(stderr, "  %s ... %s\n",
		b.c_str(),
		e.c_str()
	);

	// tokenize the begin and end commands.
	// begin may not have extra arguments.  end may have redirection.

	auto bt = tokenize(b, true);
	if (bt.size() != 1) {
		fprintf(stderr, "### Begin - Too many parameters were specified.\n");
		fprintf(stderr, "Usage - Begin\n");
		return env.status(-3);
	}

	fdmask newfds;
	int status = check_ends(e, newfds);
	newfds |= fds;
	if (status) return env.status(status);

	int rv = vector_command::execute(env, newfds);
	if (env.echo()) fprintf(stderr, "  %s\n", type == BEGIN ? "end" : ")");

	return env.status(rv);
}

namespace {



	bool evaluate(int type, const std::string &s, Environment &env) {

		auto tokens = tokenize(s, true);
		std::reverse(tokens.begin(), tokens.end());

		int32_t e;

		switch(type) {
			default: return 0;

			case ELSE_IF:
				tokens.pop_back();
			case IF:
				tokens.pop_back();
				try {
					e = evaluate_expression("If", std::move(tokens));
				}
				catch (std::exception &ex) {
					fprintf(stderr, "%s\n", ex.what());
					env.status(-5);
					e = 0;
				}
				break;

			case ELSE:
				e = 1;
				if (tokens.size() > 1) {
					fprintf(stderr, "### Else - Missing if keyword.\n");
					fprintf(stderr, "# Usage - Else [if expression...]\n");
					env.status(-3);
					e = 0;
				}
		}
		return e;
	}
}

int if_command::execute(Environment &env, const fdmask &fds) {

	int rv = 0;
	bool ok = false;

	std::string e = expand_vars(end, env);

	// parse end for indirection.
	fdmask newfds;
	int status = check_ends(e, newfds);
	newfds |= fds;
	if (status) return env.status(status);

	for (auto &c : clauses) {
		std::string s = expand_vars(c->clause, env);

		if (env.echo()) {
			if (c->type == IF) { // special.
				fprintf(stderr, "  %s ... %s\n",
					s.c_str(), e.c_str());
			}
			else {
				fprintf(stderr, "  %s\n", s.c_str());
			}
		}

		if (ok) continue;
		ok = evaluate(c->type, s, env);
		if (!ok) continue;
		rv = c->execute(env, newfds);
	}

	if (env.echo()) fprintf(stderr, "  end\n");
	return env.status(rv);
}

/*
int if_else_clause::execute() {
	return 0;
}
*/

bool if_else_clause::evaluate(const Environment &e) {
	return false;
}
