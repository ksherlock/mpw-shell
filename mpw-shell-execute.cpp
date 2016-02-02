#include "mpw-shell.h"
#include "fdset.h"
#include "value.h"

#include <cctype>
#include <cassert>
#include <cerrno>
#include <cstring>

#include <algorithm>
#include <functional>

#include <unistd.h>
#include <sys/wait.h>
#include <sysexits.h>


/*
 * Relevant shell variables (not currently supported)
 *
 *   Echo {Echo}             # control the echoing of commands to diagnostic output
 *   Echo {Exit}             # control script termination based on {Status}
 *   Echo {Test}             # don't actually run anything.
 *
 */

typedef std::vector<std::string> vs;

namespace {

	std::string &lowercase(std::string &s) {
		std::transform(s.begin(), s.end(), s.begin(), [](char c){ return std::tolower(c); });
		return s;
	}

	std::unordered_map<std::string, int (*)(const std::vector<std::string> &, const fdmask &)> builtins = {
		{"directory", builtin_directory},
		{"echo", builtin_echo},
		{"parameters", builtin_parameters},
		{"quote", builtin_quote},
		{"set", builtin_set},
		{"unset", builtin_unset},
		{"export", builtin_export},
		{"unexport", builtin_unexport},
	};

}
		
typedef std::pair<int, command_ptr> icp;



icp execute_all(command_ptr cmd);

// returns status and pointer to the next command to execute.
icp execute_if(command_ptr cmd) {

	assert(cmd && cmd->type == command_if);
	// evaluate condition...
	// skip to else or end.

	command_ptr head(cmd);

	int status = 0;

	// find the end pointer.
	// if ... end > file.text
	// redirects all output within the block.

	command_ptr end = head;
	while (end && end->type != command_end) {
		end = end->alternate.lock();
	}


	fdmask fds; // todo -- inherit from block, can be parsed from end line.



	fprintf(stdout, "    %s ... %s\n", cmd->string.c_str(), end ? end->string.c_str() : "");

	// todo -- indent levels.
	while(cmd && cmd->type != command_end) {

		int32_t e;

		std::string s = cmd->string;
		s = expand_vars(s, Environment);

		auto tokens = tokenize(s, true);

		std::reverse(tokens.begin(), tokens.end());
		e = 0;
		status = 0;
		switch(cmd->type) {
			case command_else_if:
				tokens.pop_back();
			case command_if:
				tokens.pop_back();
				try {
					e = evaluate_expression("If", std::move(tokens));
				} catch (std::exception &ex) {
					fprintf(stderr, "%s\n", ex.what());
					status = -5;
				}
				break;
			case command_else:
				e = 1;
				if (tokens.size() > 1) {
					fprintf(stderr, "### Else - Missing if keyword.\n");
					fprintf(stderr, "# Usage - Else [if expression...]\n");
					e = 0;
					status = -3;
				}
		}


		if (e) {
			command_ptr tmp;
			std::tie(status, tmp) = execute_all(cmd->next);
			break;
		}
		// skip to next condition.
		cmd = cmd->alternate.lock();
	}

	// todo -- print but don't execute remaining alternates

	// print the end tokens... [ doesn't include other tokens.]
	fprintf(stdout, "    End\n");

	return std::make_pair(status, end); // return end token -- will advance later.

}

int execute_evaluate(command_ptr cmd) {

	fdmask fds; // todo -- inherit from block.

	std::string s = cmd->string;
	s = expand_vars(s, Environment);


	fprintf(stdout, "    %s\n", s.c_str());

	auto tokens = tokenize(s, true);

	return builtin_evaluate(std::move(tokens), fds);
}


int execute_external(const std::vector<std::string> &argv, const fdmask &fds) {

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
		if (WIFSIGNALED(status)) return -1;
		fprintf(stderr, "waitpid - unexpected result\n");
		exit(EX_OSERR);
	}

}

int execute_one(command_ptr cmd) {

	if (!cmd) return 0;

	assert(cmd && cmd->type == 0);


	// todo -- before variable expansion, 
	// expand |, ||, && control structures.
	// (possibly when classifing.)

	std::string s = cmd->string;
	s = expand_vars(s, Environment);


	fprintf(stdout, "    %s\n", s.c_str());

	auto tokens = tokenize(s);

	process p;
	parse_tokens(std::move(tokens), p);



	fdmask fds = p.fds.to_mask();

	std::string name = p.arguments.front();
	lowercase(name);

	auto iter = builtins.find(name);
	if (iter != builtins.end()) {
		int status = iter->second(p.arguments, fds);
		return status;
	}


	return execute_external(p.arguments, fds);

	return 0;
}

icp execute_all(command_ptr cmd) {
	if (!cmd) return std::make_pair(0, cmd);

	int status;

	while(cmd) {

		unsigned type = cmd->type;
		switch(type)
		{
		case command_evaluate:
			status =  execute_evaluate(cmd);
			break;

		default:
			status = execute_one(cmd);
			break;

		case command_if:
			std::tie(status, cmd) = execute_if(cmd);
			break;

		case command_end:
		case command_else:
		case command_else_if:			
			return std::make_pair(status, cmd);
		}

		Environment["status"] = std::to_string(status);

		if (status != 0) {
			// only if Environment["Exit"] ? 
			throw std::runtime_error("### MPW Shell - Execution of input terminated.");
		}
		cmd = cmd->next;
	}

	return std::make_pair(status, cmd);
}

int execute(command_ptr cmd) {
	int status;
	std::tie(status, cmd) = execute_all(cmd);
	return status;
}
