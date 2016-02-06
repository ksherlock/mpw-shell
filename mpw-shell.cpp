
#include <vector>
#include <string>
#include <unordered_map>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <cerrno>
#include <signal.h>

#include "mpw-shell.h"
#include "fdset.h"

#include "phase1.h"
#include "phase2.h"
#include "command.h"

#include "mapped_file.h"
#include "error.h"

//#include <histedit.h>
#include <editline/readline.h>

#include <sys/types.h>
#include <pwd.h>
#include <sysexits.h>

//#include <uuid/uuid.h>

std::string root() {

	static std::string root;

	if (root.empty()) {
		const char *cp = getenv("HOME");
		if (!cp ||  !*cp) {
			auto pw = getpwuid(getuid());
			if (!pw) {
				fprintf(stderr,"Unable to determine home directory\n.");
				exit(EX_NOUSER);
			}
			cp = pw->pw_dir;
		}
		root = cp;
		if (root.back() != '/') root.push_back('/');
		root += "mpw/";
	}
	return root;
}
// should set {MPW}, {MPWVersion}, then execute {MPW}StartUp
void init(Environment &env) {

	env.set("mpw", root());
	env.set("status", std::string("0"));
	env.set("exit", std::string("1")); // terminate script on error.
	env.set("echo", std::string("1"));
}


int read_file(phase1 &p, const std::string &file) {
	const mapped_file mf(file, mapped_file::readonly);

	p.process(mf.begin(), mf.end(), false);
	p.finish();
	return 0;
}

int read_fd(phase1 &p, int fd) {

	unsigned char buffer[2048];
	ssize_t size;

	for (;;) {
		size = read(fd, buffer, sizeof(buffer));
		if (size == 0) break;
		if (size < 0) {
			if (errno == EINTR) continue;
			perror("read: ");
			return -1;
		}
		try {
			p.process(buffer, buffer + size);
		} catch(std::exception &ex) {
			fprintf(stderr, "%s\n", ex.what());
			p.reset();
		}
	}

	try {
		p.finish();
	} catch(std::exception &ex) {
		fprintf(stderr, "%s\n", ex.what());
		p.reset();
	}

	return 0;
}

void launch_mpw(const Environment &env, const std::vector<std::string> &argv, const fdmask &fds);

int read_make(phase1 &p1, phase2 &p2, Environment &env, const std::vector<std::string> &argv) {

	int out[2];
	int ok;


	env.set("echo", "1");
	env.set("exit", "1");

	ok = pipe(out);
	if (ok < 0) {
		perror("pipe");
		exit(EX_OSERR);
	}
	fcntl(out[0], F_SETFD, FD_CLOEXEC);
	fcntl(out[1], F_SETFD, FD_CLOEXEC);

	int child = fork();
	if (child < 0) {
		perror("fork");
		exit(EX_OSERR);
	}
	if (child == 0) {
		// child.
		fdmask fds = {-1, out[1], -1};

		launch_mpw(env, argv, fds);

		exit(EX_OSERR);
	}

	close(out[1]);
	int rv = read_fd(p1, out[0]);
	close(out[0]);
	p2.finish();

	// check for make errors.
	for(;;) {
		int status;
		int ok = waitpid(child, &status, 0);
		if (ok < 0) {
			if (errno == EINTR) continue;
			perror("waitpid: ");
			exit(EX_OSERR);
		}

		if (WIFEXITED(status)) {
			ok = WEXITSTATUS(status);
			env.status(ok, false);
			break;
		}
		if (WIFSIGNALED(status)) {
			env.status(-9, false);
			break;
		}

		fprintf(stderr, "waitpid - unexpected result\n");
		exit(EX_OSERR);
	}

	return env.status();
}

volatile int control_c = 0;
void control_c_handler(int signal, siginfo_t *sinfo, void *context) {

	// libedit gobbles up the first control-C and doesn't return until the second.
	// GNU's readline may return no the first.
	if (control_c > 3) abort();
	control_c++;
	//fprintf(stderr, "interrupt!\n");
}

int interactive(Environment &env, phase1 &p, phase2& p2) {

	std::string history_file = root();
	history_file += ".history";
	read_history(history_file.c_str());


	struct sigaction act;
	struct sigaction old_act;
	memset(&act, 0, sizeof(struct sigaction));
	sigemptyset(&act.sa_mask);

	act.sa_sigaction = control_c_handler;
	act.sa_flags = SA_SIGINFO;

	sigaction(SIGINT, &act, &old_act);

	for(;;) {
		const char *prompt = "# ";
		if (p2.continuation()) prompt = "> ";
		char *cp = readline(prompt);
		if (!cp) {
			if (control_c) {
				control_c = 0;
				fprintf(stdout, "\n");
				p.abort();
				p2.abort();
				env.status(-9, false);
				continue;
			}
			break;
		}
		control_c = 0;
		std::string s(cp);
		free(cp);

		if (s.empty()) continue;

		// don't add if same as previous entry.
		HIST_ENTRY *he = history_get(history_length);
		if (he == nullptr || s != he->line)
				add_history(s.c_str());

		s.push_back('\n');
		try {
			p.process(s);

		} catch(std::exception &ex) {
			fprintf(stderr, "%s\n", ex.what());
			p.reset();
		}

	}

	try {
		p.finish();
	} catch(std::exception &ex) {
		fprintf(stderr, "%s\n", ex.what());
		p.reset();
	}

	sigaction(SIGINT, &old_act, nullptr);

	write_history(history_file.c_str());
	fprintf(stdout, "\n");


	return 0;
}

void help() {

}
void define(Environment &env, const std::string &s) {

	auto pos = s.find('=');
	if (pos == s.npos) env.set(s, "1");
	else {
		std::string k = s.substr(0, pos);
		std::string v = s.substr(pos+1);
		env.set(k, v);
	}

}

std::string basename(const char *cp) {
	std::string tmp(cp);
	auto pos = tmp.rfind('/');
	if (pos == tmp.npos) return tmp;
	return tmp.substr(pos+1);

}

void make_help(void) {

	#undef _
	#define _(x) puts(x)

	_("Make                       # build up-to-date version of a program");
	_("Make [option...] [target...]");
	_("    -d name[=value]         # define variable name (overrides makefile definition)");
	_("    -e                      # rebuild everything regardless of dates");
	_("    -f filename             # read dependencies from specified file (default: MakeFile)");
	_("    -i dirname              # additional directory to search for include files");
#if 0
	_("    -[no]mf                 # [don't] use temporary memory (default: mf)");
#endif
	_("    -p                      # write progress information to diagnostics");
	_("    -r                      # display the roots of the dependency graph");
	_("    -s                      # display the structure of the dependency graph");
	_("    -t                      # touch dates of targets and prerequisites");
	_("    -u                      # write list of unreachable targets to diagnostics");
	_("    -v                      # write verbose explanations to diagnostics (implies -p)");
	_("    -w                      # suppress warning messages");
	_("    -y                      # like -v, but omit announcing up-to-date targets");
	_("");
	_("    --help                  # display help");
	_("    --dry-run, --test       # show what commands would run");
#undef _
}

int make(int argc, char **argv) {

	Environment e;
	init(e);

	std::vector<std::string> args;
	args.reserve(argc+1);
	bool __ = false;

	args.emplace_back("make");

	for (auto iter = argv; *iter; ++iter) {
		std::string tmp(*iter);

		if (!__) {
			if (tmp == "--")
				{ __ = true; continue; }

			if (tmp == "--help")
				{ make_help(); exit(0); }

			if (tmp == "--test" || tmp == "--dry-run") 
				{ e.set("test", "1"); continue; }
		}
		args.emplace_back(std::move(tmp));

	}

	phase1 p1;
	phase2 p2;

	p1 >>= [&p2](std::string &&s) {
		if (s.empty()) p2.finish();
		else p2(std::move(s));
	};

	p2 >>= [&e](command_ptr_vector &&v) {

		for (auto iter = v.begin(); iter != v.end(); ++iter) {
			auto &ptr = *iter;
			fdmask fds;
			try {
				ptr->execute(e, fds);
			} catch (execution_of_input_terminated &ex) {
				control_c = 0;
				if (!(ptr->terminal() && ++iter == v.end())) {
					fprintf(stderr, "%s\n", ex.what());
				}
				e.status(ex.status(), false);
				return;
			}
		}
	};


	return read_make(p1, p2, e, args);

}

int main(int argc, char **argv) {


	std::string self = basename(argv[0]);
	if (self == "mpw-make") return make(argc - 1, argv + 1);
	if (self == "mpw-shell" && argc > 1 && !strcmp(argv[1],"make")) return make(argc - 2, argv + 2);
	
	Environment e;
	init(e);

	const char *cflag = nullptr;

	int c;
	while ((c = getopt(argc, argv, "c:D:v:h")) != -1) {
		switch (c) {
			case 'c':
				// -c command
				cflag = optarg;
				break;
			case 'D':
				// -Dname or -Dname=value
				define(e, optarg);
				break;
			case 'v':
				// -v verbose
				e.set("echo", "1");
				break;
			case 'h':
				help();
				exit(0);

			default:
				help();
				exit(EX_USAGE);
		}
	}







	phase1 p1;
	phase2 p2;

	p1 >>= [&p2](std::string &&s) {
		if (s.empty()) p2.finish();
		else p2(std::move(s));
	};

	p2 >>= [&e](command_ptr_vector &&v) {

		for (auto iter = v.begin(); iter != v.end(); ++iter) {
			auto &ptr = *iter;
			fdmask fds;
			try {
				ptr->execute(e, fds);
			} catch (execution_of_input_terminated &ex) {
				control_c = 0;
				if (!(ptr->terminal() && ++iter == v.end())) {
					fprintf(stderr, "%s\n", ex.what());
				}
				e.status(ex.status(), false);
				return;
			}
		}
	};

	if (!cflag) fprintf(stdout, "MPW Shell 0.0\n");
	e.startup(true);
	read_file(p1, "/Users/kelvin/mpw/Startup");
	//p2.finish();
	e.startup(false);

	if (cflag) {
		std::string s(cflag);
		s.push_back('\n');
		p1.process(s, true);
		p2.finish();
		exit(e.status());
	}

	if (isatty(STDIN_FILENO))
		interactive(e, p1, p2);
	else 
		read_fd(p1, STDIN_FILENO);
	p2.finish();

	exit(e.status());
}
