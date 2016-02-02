
#include <vector>
#include <string>
#include <unordered_map>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <cerrno>

#include "mpw-shell.h"
#include "fdset.h"

#include "phase1.h"
#include "phase2.h"
#include "command.h"

#include "mapped_file.h"

//#include <histedit.h>
#include <editline/readline.h>

// should set {MPW}, {MPWVersion}, then execute {MPW}StartUp
void init(Environment &e) {
	e.set("status", std::string("0"));
	e.set("exit", std::string("1")); // terminate script on error.
	e.set("echo", std::string("1"));
}

int read_file(phase1 &p, const std::string &file) {
	mapped_file mf;

	mf.open(file, mapped_file::readonly);
	p.process(mf.const_begin(), mf.const_end(), true);

	return 0;
}
int read_stdin(phase1 &p) {

	unsigned char buffer[2048];
	ssize_t size;

	for (;;) {
		size = read(STDIN_FILENO, buffer, sizeof(buffer));
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

int interactive(phase1 &p) {

	for(;;) {
		char *cp = readline("# ");
		if (!cp) break;

		std::string s(cp);
		free(cp);

		if (s.empty()) continue;
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

	fprintf(stdout, "\n");
	return 0;
}

int main(int argc, char **argv) {
	
	Environment e;
	init(e);

	phase1 p1;
	phase2 p2;
	p1 >>= p2;

	p2 >>= [&e](command_ptr &&ptr) {
		fdmask fds;
		ptr->execute(e, fds);
	};
	/*
	p1 >>= [&p2](std::string &&s) {

		fprintf(stdout, " -> %s\n", s.c_str());
		p2.process(s);
	};
	*/
	read_file(p1, "/Users/kelvin/mpw/Startup");
	p2.finish();

	interactive(p1);
	p2.finish();

	return 0;
}
