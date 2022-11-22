#ifndef __mpw_shell_h__
#define __mpw_shell_h__

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

#include "environment.h"
#include "fdset.h"

const unsigned char escape = 0xb6;


class token {
public:
	enum {
		text = 0,
		eof,
		equivalent,
		not_equivalent,
		// remainder are characters.

	};
	unsigned type = text;
	std::string string;

	token() = default;
	token(token &&) = default;
	token(const token&) = default;

	token &operator=(token &&) = default;
	token &operator=(const token &) = default;

	token(const std::string &s, unsigned t = text) :
		type(t), string(s)
	{}

	token(std::string &&s, unsigned t = text) :
		type(t), string(std::move(s))
	{}

	operator std::string() const {
		return string;
	}

};



std::vector<token> tokenize(std::string &s, bool eval = false);
std::string expand_vars(const std::string &s, class Environment &, const fdmask &fds = fdmask());

//std::string quote(std::string &&s);
std::string quote(const std::string &s);


struct process;
struct value;
class fdmask;

void parse_tokens(std::vector<token> &&tokens, process &p);



int32_t evaluate_expression(Environment &e, const std::string &name, std::vector<token> &&tokens);



int read_file(Environment &e, const std::string &file, const fdmask &fds = fdmask());
int read_string(Environment &e, const std::string &s, const fdmask &fds = fdmask());
int read_fd(Environment &e, int fd, const fdmask &fds = fdmask());


#endif
