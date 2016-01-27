
#include <vector>
#include <string>
#include <unordered_map>

#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "mpw-shell.h"




std::unordered_map<std::string, EnvironmentEntry> Environment;




// should set {MPW}, {MPWVersion}, then execute {MPW}StartUp
void init(void) {
	Environment.emplace("status", std::string("0"));
	Environment.emplace("exit", std::string("1")); // terminate script on error.
}

int main(int argc, char **argv) {
	
	init();

	command_ptr head;


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

}
