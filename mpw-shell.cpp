
#include <vector>
#include <string>
#include <unordered_map>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <cerrno>
#include <signal.h>
#include <sys/wait.h>

#include "mpw-shell.h"
#include "fdset.h"

#include "macroman.h"

#include "phase1.h"
#include "phase2.h"
#include "command.h"

#include "cxx/mapped_file.h"
#include "cxx/filesystem.h"
#include "cxx/string_splitter.h"

#include "error.h"

#include <readline/readline.h>
#include <readline/history.h>

#include <sys/types.h>
#include <pwd.h>
#include <sysexits.h>
#include <paths.h>
#include <atomic>

#include "version.h"

namespace fs = filesystem;

bool utf8 = false;

fs::path root() {

	static fs::path root;

	if (root.empty()) {
		const char *cp = getenv("HOME");
		if (!cp ||  !*cp) {
			auto pw = getpwuid(getuid());
			if (!pw) {
				fprintf(stderr,"### Unable to determine home directory\n.");
				exit(EX_NOUSER);
			}
			cp = pw->pw_dir;
		}
		root = cp;
		root /= "mpw/";
		fs::error_code ec;
		fs::file_status st = status(root, ec);
		if (!fs::exists(st)) {
			fprintf(stderr, "### Warning: %s does not exist.\n", root.c_str());
		}

		else if (!fs::is_directory(st)) {
			fprintf(stderr, "### Warning: %s is not a directory.\n", root.c_str());
		}

	}
	return root;
}
// should set {MPW}, {MPWVersion}, then execute {MPW}StartUp
void init(Environment &env) {

	env.set("mpw", root());
	env.set("status", 0);
	env.set("exit", 1); // terminate script on error.
	env.set("echo", 1);
}


int read_file(phase1 &p, const std::string &file) {
	std::error_code ec;
	const mapped_file mf(file, mapped_file::readonly, ec);
	if (ec) {
		fprintf(stderr, "# Error reading %s: %s\n", file.c_str(), ec.message().c_str());
		return 0;
	}

	p.process(mf.begin(), mf.end(), false);
	p.finish();
	return 0;
}

int read_fd(phase1 &p, int fd) {

	unsigned char buffer[2048];
	ssize_t size;

	for (;;) {
		size = read(fd, buffer, sizeof(buffer));
		if (size < 0) {
			if (errno == EINTR) continue;
			perror("read: ");
			return -1;
		}
		try {
			if (size == 0) p.finish();
			else p.process(buffer, buffer + size);
		} catch(std::exception &ex) {
			fprintf(stderr, "### %s\n", ex.what());
			p.reset();
		}
		if (size == 0) break;
	}

	return 0;
}

void launch_mpw(const Environment &env, const std::vector<std::string> &argv, const fdmask &fds);
fs::path which(const Environment &env, const std::string &name);

int read_make(phase1 &p1, phase2 &p2, Environment &env, const std::vector<std::string> &argv) {

	int out[2];
	int ok;


	env.set("echo", 1);
	env.set("exit", 1);

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
	p1.finish();
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

std::atomic<int> control_c{0};
void control_c_handler(int signal, siginfo_t *sinfo, void *context) {

	// libedit gobbles up the first control-C and doesn't return until the second.
	// GNU's readline may return on the first.
	if (control_c > 3) abort();
	++control_c;
	//fprintf(stderr, "interrupt!\n");
}


int interactive(Environment &env, phase1 &p1, phase2& p2) {

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
		if (p1.continuation() || p2.continuation()) prompt = "> ";
		char *cp = readline(prompt);
		if (!cp) {
			if (control_c) {
				control_c = 0;
				fprintf(stdout, "\n");
				p1.abort();
				p2.abort();
				env.status(-9, false);
				continue;
			}
			break;
		}
		control_c = 0;
		std::string s(cp);
		free(cp);

		//if (s.empty()) continue;

		// don't add if same as previous entry.
		if (!s.empty()) {
			HIST_ENTRY *he = history_get(history_length);
			if (he == nullptr || s != he->line)
					add_history(s.c_str());
		}
		if (utf8)
			s = utf8_to_macroman(s);

		s.push_back('\n');
		try {
			p1.process(s);

		} catch(std::exception &ex) {
			fprintf(stderr, "### %s\n", ex.what());
			p1.reset();
		}

	}

	try {
		p1.finish();
	} catch(std::exception &ex) {
		fprintf(stderr, "### %s\n", ex.what());
		p1.reset();
	}

	sigaction(SIGINT, &old_act, nullptr);

	write_history(history_file.c_str());
	fprintf(stdout, "\n");


	return 0;
}

void help() {

	#undef _
	#define _(x) puts(x)

	_("MPW Shell " VERSION " (" VERSION_DATE ")");
	_("mpw-shell [option...]");
	_("    -c string               # read commands from string");
	_("    -d name[=value]         # define variable name");
	_("    -f                      # don't load MPW:Startup file");
	_("    -h                      # display help information");
	_("    -v                      # be verbose (echo = 1)");

#undef _
}

void define(Environment &env, const std::string &s) {

	auto pos = s.find('=');
	if (pos == s.npos) env.set(s, 1);
	else {
		std::string k = s.substr(0, pos);
		std::string v = s.substr(pos+1);
		env.set(k, v);
	}

}

/*
 *
 * todo:  prevent -r and -s (don't generate shell code)
 */
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


	args.push_back(""); // place-holder.

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
				fprintf(stderr, "### %s\n", ex.what());
				if (e.exit()) 
					exit(ex.status());
				e.status(ex.status(), false);
			}
		}
	};


	e.startup(true);
	read_file(p1, root() / "Startup");
	e.startup(false);

	auto path = which(e, "Make");
	if (path.empty()) {
		fputs("### MPW Shell - Command \"Make\" was not found.\n", stderr);
		return -1;
	}
	e.set("command", path);
	args[0] = path;

	return read_make(p1, p2, e, args);

}

fs::path mpw_path() {

	static fs::path path;

	if (path.empty()) {
		std::error_code ec;
		const char *cp = getenv("PATH");
		if (!cp) cp = _PATH_DEFPATH;
		std::string s(cp);
		string_splitter ss(s, ':');
		for (; ss; ++ss) {
			if (ss->empty()) continue;
			fs::path p(*ss);
			p /= "mpw";

			if (fs::is_regular_file(p, ec)) {
				path = std::move(p);
				break;
			}
		}
		//also check /usr/local/bin
		if (path.empty()) {
			fs::path p = "/usr/local/bin/mpw";
			if (fs::is_regular_file(p, ec)) {
				path = std::move(p);
			}
		}

		if (path.empty()) {
			fs::path p = root() / "bin/mpw";
			if (fs::is_regular_file(p, ec)) {
				path = std::move(p);
			}
		}

		if (path.empty()) {
			fprintf(stderr, "Unable to find mpw executable\n");
			fprintf(stderr, "PATH = %s\n", s.c_str());
			path = "mpw";
		}
	}

	return path;
}

void init_locale() {

	/*
	 * libedit assumes utf-8 if locale is C.
	 * MacRoman is en_US.  utf-8 is en_US.UTF8.
	 */


	const char *lang = getenv("LANG");
	/*
	if (lang && !strcmp(lang, "en_US")) {
		setlocale(LC_ALL, "POSIX");
	}
	*/
	utf8 = false;
	if (lang && strcasestr(lang, ".UTF-8")) {
		utf8 = true;
	}
}

int main(int argc, char **argv) {


	init_locale();

	mpw_path();

	fs::path self = fs::path(argv[0]).filename();
	if (self == "mpw-make") return make(argc - 1, argv + 1);
	if (self == "mpw-shell" && argc > 1 && !strcmp(argv[1],"make")) return make(argc - 2, argv + 2);
	
	Environment e;
	init(e);

	const char *cflag = nullptr;
	bool fflag = false;

	int c;
	while ((c = getopt(argc, argv, "c:D:vhf")) != -1) {
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
			case 'f':
				fflag = true;
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
					fprintf(stderr, "### %s\n", ex.what());
				}
				e.status(ex.status(), false);
				return;
			}
		}
	};

	if (!cflag) fprintf(stdout, "MPW Shell " VERSION "\n");
	if (!fflag) {
		fs::path startup = root() / "Startup";
		e.startup(true);
		try {
			read_file(p1, startup);
		} catch (const std::system_error &ex) {
			fprintf(stderr, "### %s: %s\n", startup.c_str(), ex.what());
		}
		e.startup(false);
	}

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
