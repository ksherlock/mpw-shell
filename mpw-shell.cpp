
#include <vector>
#include <string>
#include <unordered_map>
#include <atomic>
#include <algorithm>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <cerrno>
#include <signal.h>
#include <sys/wait.h>
#include <getopt.h>

#include "mpw-shell.h"
#include "mpw_parser.h"

#include "fdset.h"

#include "macroman.h"

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

#include "version.h"

namespace fs = filesystem;

bool utf8 = false;

fs::path home() {

	const char *cp = getenv("HOME");
	if (cp && cp) {
		auto pw = getpwuid(getuid());
		if (pw) return fs::path(pw->pw_dir);

	}
	return fs::path();
}


fs::path root() {

	static fs::path root;
	bool init = false;

	static std::array<filesystem::path, 2> locations = { {
		"/usr/share/mpw/",
		"/usr/local/share/mpw/"
	} };

	if (!init) {
		init = true;
		std::error_code ec;
		fs::path p;

		p = home();
		if (!p.empty()) {
			p /= "mpw/";
			if (fs::is_directory(p, ec)) {
				root = std::move(p);
				return root;
			}
		}
		for (fs::path p : locations) {
			p /= "mpw/";
			if (fs::is_directory(p, ec)) {
				root = std::move(p);
				return root;
			}
		}

		fprintf(stderr, "### Warning: Unable to find mpw directory.\n");
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



int read_file(Environment &e, const std::string &file, const fdmask &fds) {
	std::error_code ec;
	const mapped_file mf(file, mapped_file::readonly, ec);
	if (ec) {
		fprintf(stderr, "# Error reading %s: %s\n", file.c_str(), ec.message().c_str());
		return e.status(-1, false);
	}


	mpw_parser p(e, fds);
	e.status(0, false);

	try {
		p.parse(mf.begin(), mf.end());
		p.finish();
	} catch(const execution_of_input_terminated &ex) {
		return ex.status();
	}
	return e.status();
}


int read_string(Environment &e, const std::string &s, const fdmask &fds) {
	mpw_parser p(e, fds);
	e.status(0, false);
	try {
		p.parse(s);
		p.finish();
	} catch(const execution_of_input_terminated &ex) {
		return ex.status();
	}
	return e.status();

}


int read_fd(Environment &e, int fd, const fdmask &fds) {

	unsigned char buffer[2048];
	ssize_t size;

	mpw_parser p(e, fds);
	e.status(0, false);

	try {
		for (;;) {
			size = read(fd, buffer, sizeof(buffer));
			if (size < 0) {
				if (errno == EINTR) continue;
				perror("read");
				e.status(-1, false);
			}
			if (size == 0) break;
			p.parse(buffer, buffer + size);
		}
		p.finish();
	} catch(const execution_of_input_terminated &ex) {
		return ex.status();
	}
	return e.status();
}

void launch_mpw(const Environment &env, const std::vector<std::string> &argv, const fdmask &fds);
fs::path which(const Environment &env, const std::string &name);

int read_make(Environment &env, const std::vector<std::string> &argv) {

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
	int rv = read_fd(env, out[0]);
	close(out[0]);


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

	return rv;
}

std::atomic<int> control_c{0};
void control_c_handler(int signal, siginfo_t *sinfo, void *context) {

	// libedit gobbles up the first control-C and doesn't return until the second.
	// GNU's readline may return on the first.
	if (control_c > 3) abort();
	++control_c;
	//fprintf(stderr, "interrupt!\n");
}


int interactive(Environment &env) {

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

	mpw_parser p(env, true);


	for(;;) {
		const char *prompt = "# ";
		if (p.continuation()) prompt = "> ";
		char *cp = readline(prompt);
		if (!cp) {
			if (control_c) {
				control_c = 0;
				fprintf(stdout, "\n");
				p.abort();
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

		p.parse(s);

	}
	p.finish();

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
	int c;
	bool passthrough = false;

	static struct option longopts[] = {
		{ "help",    no_argument, nullptr, 'h' },
		{ "verbose", no_argument, nullptr, 'v' },
		{ "test",    no_argument, nullptr, 1 },
		{ "dry-run", no_argument, nullptr, 2 },
		{ nullptr, 0, nullptr, 0},
	};

	args.push_back(""); // place-holder.

	while ((c = getopt_long(argc, argv, "d:ef:i:prstuvwy", longopts, nullptr)) != -1) {
		std::string flag = "-"; flag.push_back(c);
		switch(c) {
			default:
				make_help();
				return EX_USAGE;

			case 'h':
				make_help();
				return 0;

			case 1:
				e.set("test", 1);
				break;

			case 2:
				passthrough = true;
				break;

			case 'd':
			case 'f':
			case 'i':
				args.push_back(std::move(flag));
				args.push_back(optarg);
				break;

			case 'e':
			case 'p':
			case 't':
			case 'u':
			case 'v':
			case 'w':
			case 'y':
				args.push_back(std::move(flag));
				break;

			case 'r':
			case 's':
				args.push_back(std::move(flag));
				passthrough = true;
				break;
		}


	}

	argc -= optind;
	argv += optind;
	std::transform(argv, argv+argc, std::back_inserter(args), [](const char *cp){
		return std::string(cp);
	});



	e.startup(true);
	read_file(e, root() / "Startup");
	e.startup(false);

	auto path = which(e, "Make");
	if (path.empty()) {
		fputs("### MPW Shell - Command \"Make\" was not found.\n", stderr);
		return -1;
	}
	e.set("command", path);
	args[0] = path;

	if (passthrough) {

		launch_mpw(e, args, fdmask());
		exit(EX_OSERR);
	}

	return read_make(e, args);

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
	if (self == "mpw-make") return make(argc, argv);
	if (self == "mpw-shell" && argc > 1 && !strcmp(argv[1],"make")) {
		argv[1] = (char *)"mpw-make";
		return make(argc - 1, argv + 1);
	}
	
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





	if (!cflag) fprintf(stdout, "MPW Shell " VERSION "\n");
	if (!fflag) {
		fs::path startup = root() / "Startup";
		e.startup(true);
		mpw_parser p(e);

		try {
			read_file(e, startup);
		} catch (const std::system_error &ex) {
			fprintf(stderr, "### %s: %s\n", startup.c_str(), ex.what());
		} catch (const quit_command_t &) {
		}

		e.startup(false);
	}

	try {

		int rv = 0;
		if (cflag) {
			rv = read_string(e, cflag);
			exit(rv);
		}

		if (isatty(STDIN_FILENO))
			rv = interactive(e);
		else 
			rv = read_fd(e, STDIN_FILENO);

		exit(rv);
	}
	catch (const quit_command_t &) {
		exit(0);
	}
}
