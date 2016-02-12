
#include "mpw-shell.h"

#include "fdset.h"
#include "value.h"
#include "environment.h"

#include <string>
#include <vector>
#include <algorithm>

#include <cstdio>
#include <cctype>

#include "cxx/string_splitter.h"
#include "cxx/filesystem.h"

//MAXPATHLEN
#include <sys/param.h>


namespace ToolBox {
	std::string MacToUnix(const std::string path);
	std::string UnixToMac(const std::string path);
}


namespace fs = filesystem;

namespace {

/*
	std::string &lowercase(std::string &s) {
		std::transform(s.begin(), s.end(), s.begin(), [](char c){ return std::tolower(c); });
		return s;
	}
*/
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

#define fprintf(...) dprintf(__VA_ARGS__)
inline int fputs(const char *data, int fd) {
	auto rv = write(fd, data, strlen(data));  return rv < 0 ? EOF : rv;
}

inline int fputc(int c, int fd) {
	unsigned char tmp = c;
	auto rv = write(fd, &tmp, 1);  return rv < 0 ? EOF : c;
}


int builtin_unset(Environment &env, const std::vector<std::string> &tokens, const fdmask &) {
	for (auto iter = tokens.begin() + 1; iter != tokens.end(); ++iter) {

		const std::string &name = *iter;

		env.unset(name);
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

			fprintf(stdout, "Set %s%s %s\n",
				bool(kv.second) ? "-e " : "", 
				name.c_str(), value.c_str());
		}
		return 0;
	}

	if (tokens.size() == 2) {
		std::string name = tokens[1];
		auto iter = env.find(name);
		if 	(iter == env.end()) {
			fprintf(stderr, "### Set - No variable definition exists for %s.\n", name.c_str());
			return 2;
		}

		name = quote(name);
		std::string value = quote(iter->second);
		fprintf(stdout, "Set %s%s %s\n", 
			bool(iter->second) ? "-e " : "", 
			name.c_str(), value.c_str());
		return 0;
	}

	bool exported = false;


	if (tokens.size() == 4 && tokens[1] == "-e") {
		exported = true;
	}

	if (tokens.size() > 3 && !exported) {
		fputs("### Set - Too many parameters were specified.\n", stderr);
		fputs("# Usage - set [name [value]]\n", stderr);
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
				fprintf(stderr, "### %s - \"-%c\" is not an option.\n", name, c);
				error = true;
				break;
		}
	});

	if (error) {
		fprintf(stderr, "# Usage - %s [-r | -s | name...]\n", name);
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
				fprintf(stdout, "%s%s\n", flags._s ? "" : name, quote(vname).c_str());
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
	fprintf(stderr, "### %s - Conflicting options or parameters were specified.\n", name);
	fprintf(stderr, "# Usage - %s [-r | -s | name...]\n", name);
	return 1;
}

int builtin_export(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {

	return export_common(env, true, tokens, fds);
}

int builtin_unexport(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {

	return export_common(env, false, tokens, fds);
}



int builtin_echo(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {

	//io_helper io(fds);

	bool space = false;
	bool n = false;

	for (auto iter = tokens.begin() + 1; iter != tokens.end(); ++iter) {

		const std::string &s = *iter;
		if (s == "-n" || s == "-N") {
			n = true;
			continue;
		}
		if (space) {
			fputs(" ", stdout);
		}
		fputs(s.c_str(), stdout);
		space = true;
	}
	if (!n) fputs("\n", stdout);
	return 0;
}

int builtin_quote(Environment &env, const std::vector<std::string> &tokens, const fdmask &fds) {
	// todo...

	//io_helper io(fds);

	bool space = false;
	bool n = false;

	for (auto iter = tokens.begin() + 1; iter != tokens.end(); ++iter) {

		std::string s = *iter;
		if (s == "-n" || s == "-N") {
			n = true;
			continue;
		}
		if (space) {
			fputs(" ", stdout);
		}
		s = quote(std::move(s));
		fputs(s.c_str(), stdout);
		space = true;
	}
	if (!n) fputs("\n", stdout);
	return 0;
}

int builtin_parameters(Environment &env, const std::vector<std::string> &argv, const fdmask &fds) {

	//io_helper io(fds);

	int i = 0;
	for (const auto &s : argv) {
		fprintf(stdout, "{%d} %s\n", i++, s.c_str());
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
				fprintf(stderr, "### Directory - \"-%c\" is not an option.\n", c);
				error = true;
				break;
		}
	});

	if (error) {
		fputs("# Usage - Directory [-q | directory]\n", stderr);
		return 1;
	}

	if (argv.size() > 1) {
		fputs("### Directory - Too many parameters were specified.\n", stderr);
		fputs("# Usage - Directory [-q | directory]\n", stderr);
		return 1;	
	}


	if (argv.size() == 1) {
		//cd
		if (q) {
			fputs("### Directory - Conflicting options or parameters were specified.\n", stderr);
			return 1;
		}

		// todo -- if relative path does not exist, check {DirectoryPath}
		fs::path path = ToolBox::MacToUnix(argv.front());
		std::error_code ec;
		current_path(path, ec);
		if (ec) {
			fputs("### Directory - Unable to set current directory.\n", stderr);
			fprintf(stderr, "# %s\n", ec.message().c_str());
			return 1;
		}
	}
	else {
		// pwd
		std::error_code ec;
		fs::path path = fs::current_path(ec);
		if (ec) {

			fputs("### Directory - Unable to get current directory.\n", stderr);
			fprintf(stderr, "# %s\n", ec.message().c_str());
			return 1;

		}
		// todo -- pathname translation?

		std::string s = path;
		if (!q) s = quote(std::move(s));
		fprintf(stdout, "%s\n", s.c_str());
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
					env.set(name, std::to_string(i));
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

						std::string s = std::to_string(i);
						env.set(name, s);
					}
					break;
			}
			return 0;
		}
	}

	int32_t i = evaluate_expression("Evaluate", std::move(tokens));

	// todo -- format based on -h, -o, or -b flag.
	if (output == 'h') {
		fprintf(stdout, "0x%08x\n", i);
		return 0;
	}
	if (output == 'b') {
		std::string tmp("0b");

		for (int j = 0; j < 32; ++j) {
			tmp.push_back(i & 0x80000000 ? '1' : '0');
			i <<= 1;
		}
		tmp.push_back('\n');
		fputs(tmp.c_str(), stdout);
		return 0;
	}
	if (output == 'o') {
		// octal.
		fprintf(stdout, "0%o\n", i);
		return 0;
	}
	fprintf(stdout, "%d\n", i);
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
				fprintf(stderr, "### Which - \"-%c\" is not an option.\n", c);
				error = true;
				break;
		}
	});

	if (argv.size() > 1) {
		fprintf(stderr, "### Which - Too many parameters were specified.\n");
		error = true;
	}

	if (error) {
		fprintf(stderr, "# Usage - Which [-a] [-p] [name]\n");
		return 1;
	}

	std::string s = env.get("commands");
	string_splitter ss(s, ',');

	if (argv.empty()) {
		// just print the paths.
		for (; ss; ++ss) {
			fprintf(stdout, "%s\n", ss->c_str());
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
			fprintf(stdout, "%s\n", quote(p).c_str());
			return 0;
		}
		else {
			fprintf(stderr, "### Which - File \"%s\" not found.\n", target.c_str());
			return 2;
		}
	}

	for(; ss; ++ss) {
		if (p) fprintf(stderr, "checking %s\n", ss->c_str());

		std::error_code ec;
		fs::path p(ToolBox::MacToUnix(ss->c_str()));

		p /= target;
		if (fs::exists(p, ec)) {
			found = true;
			fprintf(stdout, "%s\n", quote(p).c_str());
			if (!a) break;
		}

	}

	if (found) return 0;

	// also check built-ins?

	fprintf(stderr, "### Which - Command \"%s\" was not found.\n", target.c_str());
	return 2; // not found.
}
