#include "command.h"
#include "phase3.h"
#include "environment.h"
#include "fdset.h"
#include "builtins.h"
#include "mpw-shell.h"
#include "error.h"
#include "value.h"

#include <stdexcept>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>

#include "cxx/filesystem.h"
#include "cxx/string_splitter.h"


#include <strings.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <signal.h>
#include <atomic>

extern std::atomic<int> control_c;

namespace fs = filesystem;
extern fs::path mpw_path();

namespace ToolBox {
	std::string MacToUnix(const std::string path);
	std::string UnixToMac(const std::string path);
}


typedef std::vector<token> token_vector;


namespace {



	/*
	 * returns:
	 * <0 -> error
	 * 0 -> false
	 * >0 -> true
	 */

	int bad_if(const char *name) {
		fprintf(stderr, "### %s - Missing if keyword.\n", name);
		fprintf(stderr, "# Usage - %s [if expression...]\n", name);
		return -3;
	}

	int bad_exit() {
		fprintf(stderr, "### Exit - Missing if keyword.\n");
		fprintf(stderr, "# Usage - Exit [Number] [if expression...]\n");
		return -3;
	}

	int evaluate(int type, token_vector &&tokens, Environment &env) {
		std::reverse(tokens.begin(), tokens.end());

		int32_t e;

		switch(type) {
			default: return 0;

					// exit [number] [if expr] ([number has been removed])
			case EXIT:
			case BREAK:
			case CONTINUE:
			case ELSE:
				tokens.pop_back();

				if (tokens.empty()) return 1;

				if (strcasecmp(tokens.back().string.c_str(), "if") != 0) {
					const char *name = "";
					switch(type) {
						case BREAK: name = "Break"; break;
						case CONTINUE: name = "Continue"; break;
						case ELSE: name = "Else"; break;
						case EXIT: name = "Exit"; return bad_exit(); break;
					}
					return bad_if(name);
				}
				// fall through.

			case IF:
				tokens.pop_back();
				try {
					e = evaluate_expression("If", std::move(tokens));
				}
				catch (std::exception &ex) {
					fprintf(stderr, "%s\n", ex.what());
					return -5;
				}
				break;
		}
		return !!e;
	}


}











fs::path which(const Environment &env, const std::string &name) {
	std::error_code ec;

	if (name.find_first_of("/:") != name.npos) {
		// canonical?
		fs::path p(name);
		if (fs::exists(p, ec)) return name;
		return "";
	}

	std::string s = env.get("commands");
	for (string_splitter ss(s, ','); ss; ++ss) {
		fs::path p(ToolBox::MacToUnix(*ss));
		p /= name;
		if (fs::exists(p, ec)) return p;
	}

	// error..
	return "";
}



void launch_mpw(const Environment &env, const std::vector<std::string> &argv, const fdmask &fds) {


	std::vector<char *> cargv;
	cargv.reserve(argv.size() + 3);


	cargv.push_back((char *)"mpw");
	cargv.push_back((char *)"--shell");

	unsigned offset = cargv.size();		


	std::transform(argv.begin(), argv.end(), std::back_inserter(cargv),
		[](const std::string &s) { return strdup(s.c_str()); }
	);
	cargv.push_back(nullptr);


	// export environment...

	for (const auto &kv : env) {
		if (kv.second) { // exported
			std::string name = "mpw$" + kv.first;
			setenv(name.c_str(), kv.second.c_str(), 1);
		}
	}

	// handle any indirection...
	fds.dup();


	// re-set all signal handlers.

	/*
	 * execv modifies handlers as such:
	 * blocked -> blocked
	 * ignored -> ignored
	 * caught  -> default action
	 */
	struct sigaction sig_action;

	sig_action.sa_handler = SIG_DFL;
	sig_action.sa_flags = 0;
	sigemptyset(&sig_action.sa_mask);
	for (int i = 1; i < NSIG; ++i) {
		sigaction(i, &sig_action, NULL);
	}

	execv(mpw_path().c_str(), cargv.data());
	perror("execvp: ");
	exit(EX_OSERR); // raise a signal?
}

namespace {

	std::string &lowercase(std::string &s) {
		std::transform(s.begin(), s.end(), s.begin(), [](char c){ return std::tolower(c); });
		return s;
	}

	std::unordered_map<std::string, int (*)(Environment &, const std::vector<std::string> &, const fdmask &)> builtins = {
		{"aboutbox", builtin_aboutbox},
		{"alias", builtin_alias},
		{"catenate", builtin_catenate},
		{"directory", builtin_directory},
		{"echo", builtin_echo},
		{"execute", builtin_execute},
		{"exists", builtin_exists},
		{"export", builtin_export},
		{"help", builtin_help},
		{"parameters", builtin_parameters},
		{"quit", builtin_quit},
		{"quote", builtin_quote},
		{"set", builtin_set},
		{"shift", builtin_shift},
		{"unalias", builtin_unalias},
		{"unexport", builtin_unexport},
		{"unset", builtin_unset},
		{"version", builtin_version},
		{"which", builtin_which},

		// not in MPW.
		{"true", builtin_true},
		{"false", builtin_false},
	};



	int execute_external(const Environment &env, const std::vector<std::string> &argv, const fdmask &fds) {

		int status;
		int pid;

		struct sigaction ign, intact, quitact;
		sigset_t newsigblock, oldsigblock;

		sigemptyset(&newsigblock);
		sigaddset(&newsigblock, SIGCHLD);
		sigaddset(&newsigblock, SIGINT);
		sigaddset(&newsigblock, SIGQUIT);
		sigprocmask(SIG_BLOCK, &newsigblock, &oldsigblock);


		pid = fork();
		if (pid < 0) {
			perror("fork: ");
			exit(EX_OSERR);
		}

		if (pid == 0) {
			//sigprocmask(SIG_SETMASK, &oldsigblock, NULL);
			launch_mpw(env, argv, fds);
		}

		/* ignore int/quit while waiting on child */

		memset(&ign, 0, sizeof(ign));
		ign.sa_handler = SIG_IGN;
		sigemptyset(&ign.sa_mask);
		sigaction(SIGINT, &ign, &intact);
		sigaction(SIGQUIT, &ign, &quitact);

		for(;;) {
			pid_t ok;
			ok = waitpid(pid, &status, 0);
			if (ok < 0) {
				if (errno == EINTR) continue;
				perror("waitpid:");
				exit(EX_OSERR);
			}
			break;
		}

		sigaction(SIGINT, &intact, NULL);
		sigaction(SIGQUIT,  &quitact, NULL);
		sigprocmask(SIG_SETMASK, &oldsigblock, NULL);

		if (WIFEXITED(status)) {
			return WEXITSTATUS(status);
		}

		if (WIFSIGNALED(status)) {
			return -9; // command-. user abort exits via -9.
		}

		fprintf(stderr, "waitpid - unexpected result\n");
		exit(EX_OSERR);
	}


}


command::~command()
{}

/*
 * todo -- all errors should be handled the same, regardless of source. (throw, parse, etc).
 * echo and error should respect the active fdmask.
 */




template<class F>
int exec(std::string command, Environment &env, const fdmask &fds, bool throwup, F &&fx) {

	bool echo = true;
	int rv = 0;

	if (control_c) throw execution_of_input_terminated();

	try {
		process p;
		command = expand_vars(command, env, fds);
		auto tokens = tokenize(command, false);
		if (tokens.empty()) return 0;
		parse_tokens(std::move(tokens), p);
		env.echo("%s", command.c_str());
		echo = false;

		if (p.arguments.empty()) return env.status(0);

		rv = fx(p);
	}
	catch (mpw_error &e) {
		if (echo) env.echo("%s", command.c_str()); 
		fprintf(stderr, "### %s\n", e.what());
		return env.status(e.status(), throwup);
	}
	catch (std::exception &e) {
		if (echo) env.echo("%s", command.c_str()); 
		fprintf(stderr, "### %s\n", e.what());
		return env.status(-4, throwup);
	}

	return env.status(rv, throwup);
}

int simple_command::execute(Environment &env, const fdmask &fds, bool throwup) {


	return exec(text, env, fds, throwup, [&](process &p){

		fdmask newfds = p.fds | fds;

		std::string name = p.arguments.front();
		lowercase(name);

		auto iter = builtins.find(name);
		if (iter != builtins.end()) {
			env.set("command", name);
			int status = iter->second(env, p.arguments, newfds);
			return status;
		}

		if (env.test()) return 0;

		if (env.startup()) {
			fprintf(stderr, "### MPW Shell - startup file may not contain external commands.\n");
			return 0;
		}

		name = p.arguments.front();
		fs::path path = which(env, name);
		if (path.empty()) {
			std::string error = "MPW Shell - Command \"" + name + "\" was not found.";
			throw mpw_error(-1, error);
			//fprintf(stderr, "### MPW Shell - Command \"%s\" was not found.\n", name.c_str());
			//return -1;
		}
		env.set("command", path);
		p.arguments[0] = path;

		return execute_external(env, p.arguments, newfds);
	});
}

template<class F>
int eval_exec(std::string command, Environment &env, const fdmask &fds, bool throwup, F &&fx){

	std::string name;
	bool echo = true;
	int rv = 0;

	if (control_c) throw execution_of_input_terminated();

	try {
		command = expand_vars(command, env, fds);
		auto tokens = tokenize(command, true);

		if (tokens.empty()) return 0;
		env.echo("%s", command.c_str());
		echo = false;
		name = tokens[0].string;

		rv = fx(tokens);
	}
	catch (const exit_command_t &ex) {
		// convert to execution of input terminated.
		throw execution_of_input_terminated(ex.value);
	}

	catch (mpw_error &e) {
		if (echo) env.echo("%s", command.c_str());
		fprintf(stderr, "### %s\n", e.what());
		return env.status(e.status(), throwup);
	}

	catch(std::exception &e) {
		// these should include the argv0 name.
		if (echo) env.echo("%s", command.c_str());
		fprintf(stderr, "### %s - %s\n", name.c_str(), e.what());
		return env.status(-4, throwup);	
	}

	return env.status(rv, throwup);

}

int evaluate_command::execute(Environment &env, const fdmask &fds, bool throwup) {


	return eval_exec(text, env, fds, throwup, [&](token_vector &tokens){
		env.set("command", "evaluate");
		return builtin_evaluate(env, std::move(tokens), fds);
	});

}



int break_command::execute(Environment &env, const fdmask &fds, bool throwup) {

	return eval_exec(text, env, fds, throwup, [&](token_vector &tokens){
		env.set("command", "break");
		if (!env.loop()) throw break_error();
		int status = evaluate(BREAK, std::move(tokens), env);
		if (status > 0) throw break_command_t();
		return status;
	});

}

int continue_command::execute(Environment &env, const fdmask &fds, bool throwup) {

	return eval_exec(text, env, fds, throwup, [&](token_vector &tokens){
		env.set("command", "continue");
		if (!env.loop()) throw continue_error();
		int status = evaluate(CONTINUE, std::move(tokens), env);
		if (status > 0) throw continue_command_t();
		return status;
	});

}


int exit_command::execute(Environment &env, const fdmask &fds, bool throwup) {

	// exit
	// exit [number] [if expr ]]
	// todo -- 

	return eval_exec(text, env, fds, throwup, [&](token_vector &tokens){
		env.set("command", "exit");
		env.status(0);

		if (tokens.size() <= 1) {
			throw exit_command_t{};
		}


		int status = 0;
		value v(tokens[1]);
		// remove the return value to make processing easier.
		if (v.is_number()) {
			tokens.erase(tokens.begin() + 1);
		}
		status = evaluate(EXIT, std::move(tokens), env);

		if (status) {
			int ok = v.to_number(0);
			throw exit_command_t{ok};
		}

		return 0;
	});
}

int or_command::execute(Environment &e, const fdmask &fds, bool throwup) {

	int rv = 0;

	for (auto &c : children) {
		if (!c) continue;
		rv = c->execute(e, fds, false);
		if (rv == 0) return rv;
	}

	return e.status(rv, throwup);
}

int and_command::execute(Environment &e, const fdmask &fds, bool throwup) {

	// mpw - false && true -> no exit
	// mpw - true && false -> exit
	int rv = 0;
	for (auto &c : children) {
		if (!c) continue;
		rv = c->execute(e, fds, false);
		if (rv != 0) return rv;
	}

	return e.status(rv, throwup);
}


int pipe_command::execute(Environment &e, const fdmask &fds, bool throwup) {
	// not yet supported!
	//fputs( "### MPW Shell - Pipes are not yet supported.\n", stderr);
	//return e.status(1, throwup);

	if (children[0] && children[1]) {
		int fd;
		int rv = 0;
		char temp[32] = "/tmp/mpw-shell-XXXXXXXX";
		fdset pipe_fd;

		fd = mkstemp(temp);
		unlink(temp);
		pipe_fd.set(1, fd);

		rv = children[0]->execute(e, pipe_fd | fds, throwup);

		// fdset will close the fd ...
		pipe_fd.swap_in_out();
		lseek(fd, 0, SEEK_SET);

		rv = children[1]->execute(e, pipe_fd | fds, throwup);
		return e.status(rv, throwup);
	}

	if (children[0]) return children[0]->execute(e, fds, throwup);
	if (children[1]) return children[1]->execute(e, fds, throwup);
	return e.status(0, throwup);
}

int vector_command::execute(Environment &e, const fdmask &fds, bool throwup) {

	int rv = 0;
	for (auto &c : children) {
		if (!c) continue;
		rv = c->execute(e, fds);
	}
	return e.status(rv);
}

int error_command::execute(Environment &e, const fdmask &fds, bool throwup) {

	if (control_c) throw execution_of_input_terminated();

	if (type == ERROR) {
		fprintf(stderr, "%s\n", text.c_str());
		return e.status(-3);
	}

	std::string s = expand_vars(text, e, fds);

	e.echo("%s", s.c_str());

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

template<class F>
int begin_end_exec(std::string begin, std::string end, Environment &env, const fdmask &fds, bool throwup, F &&fx) {


	bool echo = true;
	int rv = 0;

	if (control_c) throw execution_of_input_terminated();

	try {
		process p;
		begin = expand_vars(begin, env, fds);
		end = expand_vars(end, env, fds);

		auto b = tokenize(begin, true);
		auto e = tokenize(end, false);

		parse_tokens(std::move(e), p);

		if (echo) env.echo("%s ... %s", begin.c_str(), end.c_str() );

		rv = fx(b, p);
	}
	catch (execution_of_input_terminated &e) {
		// pass through.
		throw; 
	}
	catch (mpw_error &e) {
		if (echo) env.echo("%s ... %s", begin.c_str(), end.c_str() );
		fprintf(stderr, "### %s\n", e.what());
		return env.status(e.status(), throwup);
	}
	catch (std::exception &e) {
		if (echo) env.echo("%s ... %s", begin.c_str(), end.c_str() );
		fprintf(stderr, "### %s\n", e.what());
		return env.status(-4, throwup);
	}

	return env.status(rv, throwup);
}

int begin_command::execute(Environment &env, const fdmask &fds, bool throwup) {

	return begin_end_exec(begin, end, env, fds, throwup, [&](token_vector &b, process &p){

		env.set("command", type == BEGIN ? "end" : ")");
		if (b.size() != 1) {
			fprintf(stderr, "### Begin - Too many parameters were specified.\n");
			fprintf(stderr, "Usage - Begin\n");
			return -3;
		}

		fdmask newfds = p.fds | fds;

		int rv;
		env.indent_and([&]{
			rv = vector_command::execute(env, newfds);		
		});

		env.echo("%s", type == BEGIN ? "end" : ")");

		return rv;
	});
}


int loop_command::execute(Environment &env, const fdmask &fds, bool throwup) {

	return begin_end_exec(begin, end, env, fds, throwup, [&](token_vector &b, process &p){

		env.set("command", "end");
		if (b.size() != 1) {
			fprintf(stderr, "### Loop - Too many parameters were specified.\n");
			fprintf(stderr, "Usage - Loop\n");
			return -3;
		}

		fdmask newfds = p.fds | fds;

		int rv = 0;

		for(;;) {

			if (control_c) throw execution_of_input_terminated();

			try {
				env.indent_and([&]{
					rv = vector_command::execute(env, newfds);		
				});
			}
			catch (break_command_t &ex) {
				env.echo("end");
				break;
			}
			catch (continue_command_t &ex) {}
			env.echo("end");		
		}

		return rv;
	});
}

int for_command::execute(Environment &env, const fdmask &fds, bool throwup) {

	return begin_end_exec(begin, end, env, fds, throwup, [&](token_vector &b, process &p){

		env.set("command", "end");

		if (b.size() < 3 || strcasecmp(b[2].string.c_str(), "in")) {
			fprintf(stderr, "### For - Missing in keyword.\n");
			fprintf(stderr, "Usage - For name in [word...]\n");
			return -3;
		}

		fdmask newfds = p.fds | fds;

		int rv = 0;
		for (int i = 3; i < b.size(); ++i ) {

			if (control_c) throw execution_of_input_terminated();

			env.set(b[1].string, b[i].string);

			try {
				env.loop_indent_and([&]{
					rv = vector_command::execute(env, newfds);		
				});
			}
			catch (break_command_t &ex) {
				env.echo("end");
				break;
			}
			catch (continue_command_t &ex) {}
			env.echo("end");
		}

		return rv;
	});
}





/*
 * the entire command prints even if there is an error with the expression.
 *
 */
int if_command::execute(Environment &env, const fdmask &fds, bool throwup) {


	int rv = 0;
	int error = 0;
	bool skip = false;
	bool first = true;

	fdmask newfds;
	for (auto &c : clauses) {

		int tmp;
		if (first) {
			first = false;
			tmp = begin_end_exec(c->clause, end, env, fds, false, [&](token_vector &b, process &p){

				newfds = p.fds | fds;

				int status = evaluate(c->type, std::move(b), env);
				if (status < 0) {
					error = status;
				}
				if (status > 0) {
					skip = true;

					env.indent_and([&](){
						rv = c->execute(env, newfds);
					});
				}
				return 0;
			});
			if (tmp != 0) error = tmp; 
			continue;
		}
		else {
		// second...
			tmp = eval_exec(c->clause, env, fds, false, [&](token_vector &b){
				if (skip || error) return 0;

				int status = evaluate(c->type, std::move(b), env);
				if (status < 0) {
					error = status;
				}
				if (status > 0) {
					skip = true;

					env.indent_and([&](){
						rv = c->execute(env, newfds);
					});
				}
				return 0;

			});
			if (tmp != 0 && !skip) error = tmp; 
		}
	}
	env.echo("end");
	if (error) return env.status(error, throwup);
	return env.status(rv, throwup);
}

