
#include "mpw-shell.h"

#include "fdset.h"
#include "value.h"
#include "environment.h"

#include <string>
#include <vector>
#include <algorithm>
#include <iterator>

#include <cstdio>
#include <cctype>
#include <cstring>
#include <cstdarg>

#include <unistd.h>

#include "cxx/string_splitter.h"
#include "cxx/filesystem.h"

//MAXPATHLEN
#include <sys/param.h>
#include <fcntl.h>

#include "version.h"
#include "config.h"

namespace ToolBox {
	std::string MacToUnix(const std::string path);
	std::string UnixToMac(const std::string path);
}


namespace fs = filesystem;

namespace {

	std::string &lowercase(std::string &s) {
		std::transform(s.begin(), s.end(), s.begin(), [](char c){ return std::tolower(c); });
		return s;
	}

	// doesn't handle flag arguments but builtins don't have arguments.

	template<class FX>
	std::vector<std::string> getopt(const std::vector<std::string> &argv, FX fx) {
		
		std::vector<std::string> out;
		out.reserve(argv.size());

		std::copy_if(argv.begin()+1, argv.end(), std::back_inserter(out), [&fx](const std::string &s){

			if (s.empty()) return false; // ?
			if (s.front() == '-') {
				std::for_each(s.begin() + 1, s.end(), fx);
				return false;
			}
			return true;
		});

		return out;
	}

	template <class T>
	class offset_range {
	public:

		typedef typename T::iterator iterator;
		typedef typename T::const_iterator const_iterator;

		offset_range(T &t, size_t offset) : _t(t), _offset(offset)
		{}
		offset_range(const offset_range &) = default;
		offset_range(offset_range &&) = default;
		offset_range() = delete;

		auto begin() { return std::next(_t.begin(), _offset); }
		auto begin() const { return std::next(_t.begin(), _offset); }
		auto cbegin() { return std::next(_t.begin(), _offset); }

		auto end() { return _t.end(); }
		auto end() const { return _t.end(); }
		auto cend() const { return _t.cend(); }

	private:
		T &_t;
		size_t _offset;
	};

	template<class T>
	offset_range<T> make_offset_range(T &t, size_t offset) {
		return offset_range<T>(t, offset);
	}


}



#undef stdin
#undef stdout
#undef stderr

#define stdin fds[0]
#define stdout fds[1]
#define stderr fds[2]

#undef fprintf
#undef fputs
#undef fputc
#define fprintf DO_NOT_USE_FPRINTF
#define fputs DO_NOT_USE_FPUTS
#define fputc DO_NOT_USE_FPUTC

inline int fdputs(const char *data, int fd) {
	auto rv = write(fd, data, strlen(data));
	return rv < 0 ? EOF : rv;
}

inline int fdputs(const std::string &s, int fd) {
	auto rv = write(fd, s.data(), s.size());
	return rv < 0 ? EOF : rv;
}

inline int fdputc(int c, int fd) {
	unsigned char tmp = c;
	auto rv = write(fd, &tmp, 1);
	return rv < 0 ? EOF : c;
}

#ifdef HAVE_DPRINTF
#define fdprintf dprintf
#else
inline int fdprintf(int fd, const char *format, ...) {
	char *cp = nullptr;
	va_list ap;

	va_start(ap, format);
	int len = vasprintf(&cp, format, ap);
	va_end(ap);

	fdputs(cp, fd);
	free(cp);
	return len;
}
#endif




std::vector<std::string> load_argv(Environment &env) {
	std::vector<std::string> rv;
	int n = env.pound();
	if ( n <= 0 ) return rv;
	n = std::min(n, (int)255);
	rv.reserve(n);

	for (int i = 1; i <= n; ++i) {
		rv.push_back(env.get(std::to_string(i)));
	}
	return rv;
}

int builtin_shift(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {

	int n = 1;

	if (tokens.size() > 3) {
		fdputs("### Shift - Too many parameters were specified.\n", stderr);
		fdputs("# Usage - Shift [number]\n", stderr);
		return 1;
	}

	if (tokens.size() == 2) {

		value v(tokens[1]);
		if (v.is_number() && v.number >= 0) n = v.number;
		else {
			fdputs("### Shift - The parameter must be a positive number.\n", stderr);
			fdputs("# Usage - Shift [number]\n", stderr);
			return 1;
		}
	}

	if (n == 0) return 0;

	auto argv = load_argv(env);
	if (argv.empty()) return 0;

	std::move(argv.begin() + n , argv.end(), argv.begin());
	do {
		env.unset(std::to_string(argv.size()));
		argv.pop_back();
	} while (--n);

	env.set_argv(argv);

	return 0;
}



int builtin_unset(Environment &env, const std::vector<std::string> &tokens, const fdmask &) {

	for (const auto &s : make_offset_range(tokens, 1)) {
		env.unset(s);
	}

	// unset [no arg] removes ALL variables
	if (tokens.size() == 1) {
		env.unset();
	}
	return 0;
}

int builtin_set(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {
	// set var name  -- set
	// set var -- just print the value

	// 3.5 supports -e to also export it.

	//io_helper io(fds);


	if (tokens.size() == 1) {

		for (const auto &kv : env) {
			std::string name = quote(kv.first);
			std::string value = quote(kv.second);

			fdprintf(stdout, "Set %s%s %s\n",
				bool(kv.second) ? "-e " : "", 
				name.c_str(), value.c_str());
		}
		return 0;
	}

	if (tokens.size() == 2) {
		std::string name = tokens[1];
		auto iter = env.find(name);
		if 	(iter == env.end()) {
			fdprintf(stderr, "### Set - No variable definition exists for %s.\n", name.c_str());
			return 2;
		}

		name = quote(name);
		std::string value = quote(iter->second);
		fdprintf(stdout, "Set %s%s %s\n", 
			bool(iter->second) ? "-e " : "", 
			name.c_str(), value.c_str());
		return 0;
	}

	bool exported = false;


	if (tokens.size() == 4 && tokens[1] == "-e") {
		exported = true;
	}

	if (tokens.size() > 3 && !exported) {
		fdputs("### Set - Too many parameters were specified.\n", stderr);
		fdputs("# Usage - set [name [value]]\n", stderr);
		return 1;
	}

	std::string name = tokens[1+exported];
	std::string value = tokens[2+exported];

	env.set(name, value, exported);
	return 0;
}



static int export_common(Environment &env, bool export_or_unexport, const std::vector<std::string> &tokens, const fdmask &fds) {

	const char *name = export_or_unexport ? "Export" : "Unexport";

	struct {
		int _r = 0;
		int _s = 0;
	} flags;
	bool error = false;

	std::vector<std::string> argv = getopt(tokens, [&](char c){
		switch(c) {
			case 'r':
			case 'R':
				flags._r = true;
				break;
			case 's':
			case 'S':
				flags._s = true;
				break;
			default:
				fdprintf(stderr, "### %s - \"-%c\" is not an option.\n", name, c);
				error = true;
				break;
		}
	});

	if (error) {
		fdprintf(stderr, "# Usage - %s [-r | -s | name...]\n", name);
		return 1;
	}

	if (argv.empty()) {
		if (flags._r && flags._s) goto conflict;

		// list of exported vars.
		// -r will generate unexport commands for exported variables.
		// -s will only print the names.


		name = export_or_unexport ? "Export " : "Unexport ";

		for (const auto &kv : env) {
			const std::string& vname = kv.first;
			if (kv.second == export_or_unexport)
				fdprintf(stdout, "%s%s\n", flags._s ? "" : name, quote(vname).c_str());
		}
		return 0;
	}
	else {
		// mark as exported.

		if (flags._r || flags._s) goto conflict;

		for (std::string s : argv) {
			auto iter = env.find(s);
			if (iter != env.end()) iter->second = export_or_unexport;
		}	
		return 0;
	}

conflict:
	fdprintf(stderr, "### %s - Conflicting options or parameters were specified.\n", name);
	fdprintf(stderr, "# Usage - %s [-r | -s | name...]\n", name);
	return 1;
}

int builtin_export(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {

	return export_common(env, true, tokens, fds);
}

int builtin_unexport(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {

	return export_common(env, false, tokens, fds);
}

int builtin_alias(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {

	// alias -> lists all aliases
	// alias name -> list single alias
	// alias name parms... -> add a new alias.

	if (tokens.size() == 1) {
		for (const auto &p : env.aliases()) {
			fdprintf(stdout, "Alias %s %s\n", quote(p.first).c_str(), quote(p.second).c_str());
		}
		return 0;
	}

	std::string name = tokens[1];
	if (tokens.size() == 2) {
		const auto as = env.find_alias(name);
		if (as.empty()) {
			fdprintf(stderr, "### Alias - No alias exists for %s\n", quote(name).c_str());
			return 1;
		}
		fdprintf(stdout, "Alias %s %s\n", quote(name).c_str(), quote(as).c_str());
		return 0;
	}		

	std::string as;
	for (const auto &s : make_offset_range(tokens, 2)) {
		as += s;
		as.push_back(' ');
	}
	as.pop_back();

	// add/remove it to the alias table...

	if (as.empty()) {
		env.remove_alias(name);
	}	
	else {
		env.add_alias(std::move(name), std::move(as));
	}
	return 0;
}

int builtin_unalias(Environment &env, const std::vector<std::string> &tokens, const fdmask &) {
	// unalias -> remove all aliases.
	// unalias name -> remove single alias.
	if (tokens.size() == 1) {
		env.remove_alias();
		return 0;
	}
	for (const auto &x : make_offset_range(tokens, 1)) {
		env.remove_alias(x);
	}
	return 0;
}

int builtin_echo(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {

	//io_helper io(fds);

	bool space = false;
	bool n = false;

	for (const auto &s : make_offset_range(tokens, 1)) {
		if (s == "-n" || s == "-N") {
			n = true;
			continue;
		}
		if (space) {
			fdputs(" ", stdout);
		}
		fdputs(s.c_str(), stdout);
		space = true;
	}
	if (!n) fdputs("\n", stdout);
	return 0;
}

int builtin_quote(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {
	// todo...

	//io_helper io(fds);

	bool space = false;
	bool n = false;

	for (const auto &s : make_offset_range(tokens, 1)) {
		if (s == "-n" || s == "-N") {
			n = true;
			continue;
		}
		if (space) {
			fdputs(" ", stdout);
		}
		fdputs(quote(s).c_str(), stdout);
		space = true;
	}
	if (!n) fdputs("\n", stdout);
	return 0;
}

int builtin_parameters(Environment &env, const std::vector<std::string> &argv, const fdmask &fds) {

	//io_helper io(fds);

	int i = 0;
	for (const auto &s : argv) {
		fdprintf(stdout, "{%d} %s\n", i++, s.c_str());
	}
	return 0;
}


int builtin_directory(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {
	// directory [-q]
	// directory path

	// for relative names, uses {DirectoryPath} (if set) rather than .
	// set DirectoryPath ":,{MPW},{MPW}Projects:"

	/*
	 * Parameters:
	 * ----------
	 * directory
	 * Sets the default directory to directory. If you specify directory
	 * as a leafname (that is, the final portion of a full pathname), the
	 * MPW Shell searches for the directory in the current directory path
	 * (for example, searching "{MPW}Examples:" for CExamples). However, if
	 * the MPW Shell fails to find the directory in the current directory
	 * path, it searches the directories listed in the {DirectoryPath} MPW
	 * Shell variable, which contains a list of directories to be searched
	 * in order of precedence. The last example illustrates how to do this.
	 *
	 * Options:
	 * -------
	 * -q
	 * Inhibits quoting the directory pathname written to standard
	 * output. This option applies only if you omit the directory
	 * parameter Normally the MPW Shell quotes the current default
	 * directory name if it contains spaces or other special characters
	 *
	 * Status:
	 * ------
	 * Directory can return the following status codes:
	 *
	 * 0 no errors
	 * 1 directory not found; command aborted; or parameter error
	 *
	 */



	//io_helper io(fds);

	bool q = false;
	bool error = false;

	std::vector<std::string> argv = getopt(tokens, [&](char c){
		switch(c)
		{
			case 'q':
			case 'Q':
				q = true;
				break;
			default:
				fdprintf(stderr, "### Directory - \"-%c\" is not an option.\n", c);
				error = true;
				break;
		}
	});

	if (error) {
		fdputs("# Usage - Directory [-q | directory]\n", stderr);
		return 1;
	}

	if (argv.size() > 1) {
		fdputs("### Directory - Too many parameters were specified.\n", stderr);
		fdputs("# Usage - Directory [-q | directory]\n", stderr);
		return 1;	
	}


	if (argv.size() == 1) {
		//cd
		if (q) {
			fdputs("### Directory - Conflicting options or parameters were specified.\n", stderr);
			return 1;
		}

		// todo -- if relative path does not exist, check {DirectoryPath}
		fs::path path = ToolBox::MacToUnix(argv.front());
		std::error_code ec;
		current_path(path, ec);
		if (ec) {
			fdputs("### Directory - Unable to set current directory.\n", stderr);
			fdprintf(stderr, "# %s\n", ec.message().c_str());
			return 1;
		}
	}
	else {
		// pwd
		std::error_code ec;
		fs::path path = fs::current_path(ec);
		if (ec) {

			fdputs("### Directory - Unable to get current directory.\n", stderr);
			fdprintf(stderr, "# %s\n", ec.message().c_str());
			return 1;

		}
		// todo -- pathname translation?

		std::string s = path;
		if (!q) s = quote(std::move(s));
		fdprintf(stdout, "%s\n", s.c_str());
	}
	return 0;
}


int builtin_exists(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {

	bool _a = false; // print if alias/symlink
	bool _d = false; // print if directory
	bool _f = false; // print if normal file
	bool _n = false; // don't follow alias
	bool _w = false; // print if normal file + writable
	bool _q = false; // don't quote names
	bool error = false;

	std::vector<std::string> argv = getopt(tokens, [&](char c){
		switch(tolower(c))
		{
			case 'a':
				_a = true;
				break;
			case 'd':
				_d = true;
				break;
			case 'f':
				_f = true;
				break;
			case 'n':
				_n = true;
				break;
			case 'q':
				_q = true;
				break;
			case 'w':
				_w = true;
				break;
			default:
				fdprintf(stderr, "### Exists - \"-%c\" is not an option.\n", c);
				error = true;
				break;
		}
	});	

	if (_w) _f = false;

	if (_a + _d + _f + _w > 1) {
		fdputs("### Exists - Conflicting options were specified.\n", stderr);
		error = true;
	}

	if (argv.size() < 1) {
		fdputs("### Exists - Not enough parameters were specified.\n", stderr);
		error = true;
	}



	if (error) {
		fdputs("# Usage - Exists [-a | -d | -f | -w] [-n] [-q] name...\n", stderr);
		return 1;
	}

	for (auto &s : argv) {
		std::string path = ToolBox::MacToUnix(s);
		std::error_code ec;
		fs::file_status st = _n ? fs::symlink_status(path, ec) : fs::status(path, ec);
		if (ec) continue;

		if (_d && !fs::is_directory(st)) continue;
		if (_a && !fs::is_symlink(st)) continue;
		if (_f && !fs::is_regular_file(st)) continue;
		if (_w && !fs::is_regular_file(st)) continue;
		if (_w && (access(path.c_str(), W_OK | F_OK) < 0)) continue; 

		if (!_q) s = quote(std::move(s));
		fdprintf(stdout, "%s\n", s.c_str());

	}


	return 0;
}

static bool is_assignment(int type) {
	switch(type)
	{
		case '=':
		case '+=':
		case '-=':
			return true;
		default:
			return false;
	}
}

int builtin_evaluate(Environment &env, std::vector<token> &&tokens, const fdmask &fds) {
	// evaluate expression
	// evaluate variable = expression
	// evaluate variable += expression
	// evaluate variable -= expression

	// flags -- -h -o -b -- print in hex, octal, or binary

	// convert the arguments to a stack.


	int output = 'd';

	//io_helper io(fds);

	std::reverse(tokens.begin(), tokens.end());

	// remove 'Evaluate'
	tokens.pop_back();

	// check for -h -x -o
	if (tokens.size() >= 2 && tokens.back().type == '-') {

		const token &t = tokens[tokens.size() - 2];
		if (t.type == token::text && t.string.length() == 1) {
			int flag = tolower(t.string[0]);
			switch(flag) {
				case 'o':
				case 'h':
				case 'b':
					output = flag;
					tokens.pop_back();
					tokens.pop_back();
			}
		}

	}

	if (tokens.size() >= 2 && tokens.back().type == token::text)
	{
		int type = tokens[tokens.size() -2].type;

		if (is_assignment(type)) {

			std::string name = tokens.back().string;

			tokens.pop_back();
			tokens.pop_back();

			int32_t i = evaluate_expression("Evaluate", std::move(tokens));

			switch(type) {
				case '=':
					env.set(name, i);
					break;
				case '+=':
				case '-=':
					{
						value old;
						auto iter = env.find(name);
						if (iter != env.end()) old = (const std::string &)iter->second;

						switch(type) {
							case '+=':
								i = old.to_number() + i;
								break;
							case '-=':
								i = old.to_number() - i;
								break;
						}

						env.set(name, i);
					}
					break;
			}
			return 0;
		}
	}

	int32_t i = evaluate_expression("Evaluate", std::move(tokens));

	if (output == 'h') {
		fdprintf(stdout, "0x%08x\n", i);
		return 0;
	}

	if (output == 'b') {
		std::string tmp("0b");

		for (int j = 0; j < 32; ++j) {
			tmp.push_back(i & 0x80000000 ? '1' : '0');
			i <<= 1;
		}
		tmp.push_back('\n');
		fdputs(tmp.c_str(), stdout);
		return 0;
	}

	if (output == 'o') {
		// octal.
		fdprintf(stdout, "0%o\n", i);
		return 0;
	}

	fdprintf(stdout, "%d\n", i);
	return 0;
}

int builtin_which(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {

	// which [-a] [-p] [command]
	bool a = false;
	bool p = false;
	bool error = false;

	std::vector<std::string> argv = getopt(tokens, [&](char c){
		switch(c)
		{
			case 'a': case 'A': a = true; break;
			case 'p': case 'P': p = true; break;

			default:
				fdprintf(stderr, "### Which - \"-%c\" is not an option.\n", c);
				error = true;
				break;
		}
	});

	if (argv.size() > 1) {
		fdprintf(stderr, "### Which - Too many parameters were specified.\n");
		error = true;
	}

	if (error) {
		fdprintf(stderr, "# Usage - Which [-a] [-p] [name]\n");
		return 1;
	}

	std::string s = env.get("commands");
	string_splitter ss(s, ',');

	if (argv.empty()) {
		// just print the paths.
		for (; ss; ++ss) {
			fdprintf(stdout, "%s\n", ss->c_str());
		}
		return 0;
	}
	std::string target = argv[0];
	

	bool found = false;

	// if absolute or relative path, check that.
	if (target.find_first_of("/:") != target.npos) {

		std::error_code ec;
		fs::path p(ToolBox::MacToUnix(target));

		if (fs::exists(p, ec)) {
			fdprintf(stdout, "%s\n", quote(p).c_str());
			return 0;
		}
		else {
			fdprintf(stderr, "### Which - File \"%s\" not found.\n", target.c_str());
			return 2;
		}
	}

	for(; ss; ++ss) {
		if (p) fdprintf(stderr, "checking %s\n", ss->c_str());

		std::error_code ec;
		fs::path p(ToolBox::MacToUnix(ss->c_str()));

		p /= target;
		if (fs::exists(p, ec)) {
			found = true;
			fdprintf(stdout, "%s\n", quote(p).c_str());
			if (!a) break;
		}

	}

	// check builtins...
	if (!found || a) {

		static const char *builtins[] = {
			"aboutbox",
			"alias",
			"catenate",
			"directory",
			"echo",
			"exists",
			"export",
			"parameters",
			"quote",
			"set",
			"shift",
			"unalias",
			"unexport",
			"unset",
			"version",
			"which",
		};

		lowercase(target);

		auto iter = std::find(std::begin(builtins), std::end(builtins), target);
		if (iter != std::end(builtins)) {
			fdprintf(stdout, "%s\n", *iter);
			found = true;
		}

	}

	if (found) return 0;

	// also check built-ins?

	fdprintf(stderr, "### Which - Command \"%s\" was not found.\n", target.c_str());
	return 2; // not found.
}

int cat_helper(int in, int out) {
	static uint8_t buffer[4096];

	for(;;) {
		ssize_t rcount = read(in, buffer, sizeof(buffer));
		if (rcount < 0) {
			if (errno == EINTR) continue;
			return 2;
		}

		if (rcount == 0) break;

		for (;;) {
			ssize_t wcount = write(out, buffer, rcount);
			if (wcount < 0) {
				if (errno == EINTR) continue;
				return 2;	
			}
			if (wcount != rcount) return 2;
			break;
		}
	}
	return 0;
}


int builtin_catenate(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {

	if (tokens.size() == 1) {
		int rv = cat_helper(stdin, stdout);
		if (rv) fdputs("### Catenate - I/O Error\n", stderr);
		return rv;
	}

	for (const auto &s : make_offset_range(tokens, 1)) {

		std::string path = ToolBox::MacToUnix(s);
		int fd = open(path.c_str(), O_RDONLY);
		if (fd < 0) {
			fdprintf(stderr, "### Catenate - Unable to open \"%s\".\n", path.c_str());
			return 1;
		}

		int rv = cat_helper(fd, stdout);
		close(fd);
		if (rv) {
			fdputs("### Catenate - I/O Error\n", stderr);
			return rv;
		}
	}
	return 0;
}

int builtin_version(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {


	bool _v = false;
	bool error = false;

	auto argv = getopt(tokens, [&](char c){
		switch(tolower(c))
		{
			case 'v':
				_v = true;
				break;
			default:
				fdprintf(stderr, "### Version - \"-%c\" is not an option.\n", c);
				error = true;
				break;
		}
	});

	if (argv.size() != 0) {
		fdprintf(stderr, "### Version - Too many parameters were specified.\n");
		error = true;
	}

	if (error) {
		fdprintf(stderr, "# Usage - Version [-v]\n");
		return 1;
	}

	//fdputs("MPW Shell 3.5, Copyright Apple Computer, Inc. 1985-99. All rights reserved.\n", stdout);
	fdputs("MPW Shell " VERSION ", Copyright Kelvin W Sherlock 2016. All rights reserved.\n", stdout);
	fdputs("based on MPW Shell 3.5, Copyright Apple Computer, Inc. 1985-99. All rights reserved.\n", stdout);

	if (_v) {
		fdputs("This version built on " __DATE__ " at " __TIME__ ".\n", stdout);
	}

	return 0;
}

int builtin_aboutbox(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {
	// the most important command of all!

	if (tokens.size() == 2 && tokens[1] == "--moof") {

		fdputs(
		"\n"
		"              ##                                            \n"
		"            ##  ##  ####                                    \n"
		"            ##  ####  ##                                    \n"
		"          ##          ##                                    \n"
		"        ##    ##    ##                              ##      \n"
		"      ##            ##                              ####    \n"
		"    ##                ##                          ##  ##    \n"
		"      ########          ####                    ##    ##    \n"
		"              ##            ####################      ##    \n"
		"              ##            ##############          ##      \n"
		"                ####          ############        ##        \n"
		"                ######            ######          ##        \n"
		"                ######                          ##          \n"
		"                ####                            ##          \n"
		"                ##                              ##          \n"
		"                ##      ################        ##          \n"
		"                ##    ##                ##        ##        \n"
		"                ##    ##                  ##      ##        \n"
		"                ##    ##                    ##    ##        \n"
		"                ##    ##                    ##    ##        \n"
		"              ##    ##                    ##    ##          \n"
		"              ######                      ######            \n"
		"\n"
		,stdout);

		return 0;
	}


	fdprintf(stdout,
"+--------------------------------------+\n"
"| MPW Shell %-4s                       |\n"
"|                                      |\n"
"|                                      |\n"
"| (c) 2016 Kelvin W Sherlock           |\n"
"+--------------------------------------+\n"
	, VERSION);
	return 0;
}


int builtin_true(Environment &, const std::vector<std::string> &, const fdmask &) {
	return 0;
}

int builtin_false(Environment &, const std::vector<std::string> &, const fdmask &) {
	return 1;
}
