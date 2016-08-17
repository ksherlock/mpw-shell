#include <algorithm>

#include "mpw_parser.h"
#include "phase3_parser.h"

#include "command.h"
#include "error.h"

mpw_parser::mpw_parser(Environment &e, fdmask fds, bool interactive) : _env(e), _fds(fds), _interactive(interactive)
{

	_p3 = phase3::make();
	_p2.set_next([this](int type, std::string &&s){
		_p3->parse(type, std::move(s));
	});

	_p1.set_next([this](std::string &&s){
		_p2.parse(std::move(s));
	});
}


mpw_parser::~mpw_parser() {

}


bool mpw_parser::continuation() const {
	if (_p1.continuation()) return true;
	if (_p2.continuation()) return true;
	if (_p3->continuation()) return true;
	return false;
}

void mpw_parser::finish() {
	_p1.finish();
	_p2.finish();
	_p3->parse(0, "");

	// and now execute the commands...
	execute();
}


void mpw_parser::reset() {
	_p1.reset();
	_p2.reset();
	_p3->reset();
	_abort = false;
}

void mpw_parser::abort() {
	_p1.abort();
	_p2.abort();
	//_p3->abort();
	_p3->reset();
	_abort = true;
}


void mpw_parser::parse(const void *begin, const void *end) {
	if (_abort) return;
	_p1.parse((const unsigned char *)begin, (const unsigned char *)end);

	// and execute...
	execute();
}


void mpw_parser::execute() {
	if (_abort) {
		_p3->command_queue.clear();
		return;
	}

	auto commands = std::move(_p3->command_queue);
	_p3->command_queue.clear();


	std::reverse(commands.begin(), commands.end());

	command_ptr cmd;

	try {
		while (!commands.empty()) {
			cmd = std::move(commands.back());
			commands.pop_back();
			cmd->execute(_env, _fds);
		}

	} catch (execution_of_input_terminated &ex) {

		_env.status(ex.status(), false);

		if (_interactive) {
			if (!cmd->terminal() || !commands.empty()) {
				fprintf(stderr, "### %s\n", ex.what());
			}
			return;
		}
		_abort = true;
		throw;
	}

}