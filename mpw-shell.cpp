
#include <vector>
#include <string>
#include <unordered_map>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <cerrno>

#define NOCOMMAND
#include "mpw-shell.h"

#include "phase1.h"
#include "phase2.h"
#include "command.h"


std::unordered_map<std::string, EnvironmentEntry> Environment;




// should set {MPW}, {MPWVersion}, then execute {MPW}StartUp
void init(void) {
	Environment.emplace("status", std::string("0"));
	Environment.emplace("exit", std::string("1")); // terminate script on error.
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

int main(int argc, char **argv) {
	
	init();

	command_ptr head;


	phase1 p1;
	phase2 p2;
	p1 >>= p2;

	p2 >>= [](command_ptr &&ptr) {
		printf("command: %d\n", ptr->type);
		ptr->execute();
	};
	/*
	p1 >>= [&p2](std::string &&s) {

		fprintf(stdout, " -> %s\n", s.c_str());
		p2.process(s);
	};
	*/
	read_stdin(p1);
	p2.finish();

/*
	try {
		head = read_fd(0);
	} catch (std::exception &ex) {
		fprintf(stderr, "%s\n", ex.what());
		exit(1);
	}


	try {
		int status = execute(head);
		exit(status);
	} catch(std::exception &ex) {
		fprintf(stderr, "%s\n", ex.what());
		exit(1);
	}
*/
}
