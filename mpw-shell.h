#ifndef __mpw_shell_h__
#define __mpw_shell_h__

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>

class command;
typedef std::shared_ptr<command> command_ptr;
typedef std::weak_ptr<command> weak_command_ptr;

const unsigned char escape = 0xb6;

// environment has a bool which indicates if exported.
struct EnvironmentEntry {
public:
	operator bool() const { return exported; }
	operator bool&() { return exported; }

	operator const std::string&() const { return value; }
	operator std::string&() { return value; }

	EnvironmentEntry() = default;
	EnvironmentEntry(const EnvironmentEntry &) = default;
	EnvironmentEntry(EnvironmentEntry &&) = default;

	EnvironmentEntry(const std::string &s, bool e = false) : value(s), exported(e)
	{}
	EnvironmentEntry(std::string &&s, bool e = false) : value(std::move(s)), exported(e)
	{}

	~EnvironmentEntry() = default;

	EnvironmentEntry& operator=(bool &rhs) { exported = rhs; return *this; }
	EnvironmentEntry& operator=(const std::string &rhs) { value = rhs; return *this; }
	EnvironmentEntry& operator=(const EnvironmentEntry &) = default;
	EnvironmentEntry& operator=(EnvironmentEntry &&) = default;

private:
	std::string value;
	bool exported = false;

};

extern std::unordered_map<std::string, EnvironmentEntry> Environment;

enum {
	command_if = 1,
	command_else,
	command_else_if,
	command_end,
	command_begin,
	command_evaluate,
};

class command {
	public:
	unsigned type = 0;
	unsigned line = 0;
	unsigned level = 0;

	std::string string;
	command_ptr next;
	weak_command_ptr alternate; // if -> else -> end. weak to prevent cycles.

	command() = default;
	command(command &&) = default;
	command(const command &) = default;

	command(unsigned t, const std::string &s) :
		type(t), string(s)
	{}

	command(unsigned t, std::string &&s) :
		type(t), string(std::move(s))
	{}

	command(const std::string &s) : string(s)
	{}

	command(std::string &&s) : string(std::move(s))
	{}


};


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


command_ptr read_fd(int fd);
command_ptr read_file(const std::string &);
command_ptr read_string(const std::string &);

std::vector<token> tokenize(const std::string &s, bool eval = false);
std::string expand_vars(const std::string &s, const std::unordered_map<std::string, EnvironmentEntry> &env);

//std::string quote(std::string &&s);
std::string quote(const std::string &s);


struct process;
struct value;
class fdmask;

void parse_tokens(std::vector<token> &&tokens, process &p);


int execute(command_ptr cmd);

int32_t evaluate_expression(const std::string &name, std::vector<token> &&tokens);

int builtin_directory(const std::vector<std::string> &, const fdmask &);
int builtin_echo(const std::vector<std::string> &, const fdmask &);
int builtin_parameters(const std::vector<std::string> &, const fdmask &);
int builtin_quote(const std::vector<std::string> &tokens, const fdmask &);
int builtin_set(const std::vector<std::string> &, const fdmask &);
int builtin_unset(const std::vector<std::string> &, const fdmask &);
int builtin_export(const std::vector<std::string> &, const fdmask &);
int builtin_unexport(const std::vector<std::string> &, const fdmask &);

int builtin_evaluate(std::vector<token> &&, const fdmask &);


#endif
