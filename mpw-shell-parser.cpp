#include "mpw-shell.h"
#include "fdset.h"
#include "value.h"

#include <unistd.h>
#include <fcntl.h>
#include <algorithm>

/*
 * I'm sick of fighting with lemon.  Just generate it by hand.
 *
 */



namespace ToolBox {
	std::string MacToUnix(const std::string path);
	std::string UnixToMac(const std::string path);
}

template<class T>
T pop(std::vector<T> &v) {
	T t = std::move(v.back());
	v.pop_back();
	return t;
}

/*
### MPW Shell - Unable to open "nofile".
# File not found (OS error -43)
{status} = -4
*/

void open_error(const std::string &name) {

	std::string error = "### MPW Shell - Unable to open ";
	error.push_back('"');
	error.append(name);
	error.push_back('"');
	error.push_back('.');
	throw std::runtime_error(error);	
}

int open(const std::string &name, int flags) {

	// dup2 does not copy the O_CLOEXEC flag so it's safe to use.

	std::string uname = ToolBox::MacToUnix(name);

	int fd = ::open(uname.c_str(), flags | O_CLOEXEC, 0666);
	if (fd < 0) {
		open_error(name);
		return -1;
	}
	return fd;
}

void parse_tokens(std::vector<token> &&tokens, process &p) {


	fdset fds;
	std::vector<std::string> argv;

	std::reverse(tokens.begin(), tokens.end());
	argv.reserve(tokens.size());

	// first token is always treated as a string.
	token t = pop(tokens);
	argv.emplace_back(std::move(t.string));

	while(!tokens.empty()) {

		t = pop(tokens);

		int flags;
		unsigned fd_bits;

		switch (t.type) {

			// >,  >>  -- redirect stdout.
			case '>':
				flags = O_WRONLY | O_CREAT | O_TRUNC;
				fd_bits = 1 << 1;
				goto redir;

			case '>>':
				flags = O_WRONLY | O_CREAT | O_APPEND;
				fd_bits = 1 << 1;
				goto redir;

			case '<':
				flags = O_RDONLY;
				fd_bits = 1 << 0;
				goto redir;

			// ∑, ∑∑ - redirect stdout & stderr
			case 0xb7: // ∑
				flags = O_WRONLY | O_CREAT | O_TRUNC;
				fd_bits = (1 << 1) + (1 << 2);
				goto redir;

			case 0xb7b7: // ∑∑
				flags = O_WRONLY | O_CREAT | O_APPEND;
				fd_bits = (1 << 1) + (1 << 2);
				goto redir;

			// ≥,  ≥≥  -- redirect stdout.
			case 0xb3: // ≥
				flags = O_WRONLY | O_CREAT | O_TRUNC;
				fd_bits = 1 << 2;
				goto redir;

			case 0xb3b3: // ≥≥
				flags = O_WRONLY | O_CREAT | O_APPEND;
				fd_bits = 1 << 2;
				goto redir;

			redir:

				{
					if (tokens.empty()) {
						throw std::runtime_error("### MPW Shell - Missing file name.");
					}
					token name = pop(tokens);
					int fd = open(name.string, flags);


					// todo -- if multiple fd_bits (stdin+stderr, should dup the second fd?)
					switch(fd_bits) {
					case 1 << 0:
						fds.set(0, fd);
						break;
					case 1 << 1:
						fds.set(1, fd);
						break;
					case 1 << 2:
						fds.set(2, fd);
						break;

					case (1 << 1) + (1 << 2):
						{
							int newfd = fcntl(fd, F_DUPFD_CLOEXEC);
							if (newfd < 1) {
								close(fd);
								open_error(name);
							}
							fds.set(1, fd);
							fds.set(2, newfd);
							break;
						}
					default:
						// ???
						close(fd);
					}
				}
				break;


			default:
				argv.emplace_back(std::move(t.string));
				break;
		}
	}

	p.arguments = std::move(argv);
	p.fds = std::move(fds);
}


class expression_parser {

public:

	expression_parser(const std::string &n, std::vector<token> &&t) : 
		name(n), tokens(std::move(t))
	{}

	expression_parser(const expression_parser &) = delete;
	expression_parser(expression_parser &&) = delete;

	expression_parser& operator=(const expression_parser &) = delete;
	expression_parser& operator=(expression_parser &&) = delete;

	// returns integer value of the expression.
	int32_t evaluate();


private:

	value terminal();
	value unary();
	value binary();


	value eval(int op, value &lhs, value &rhs);

	[[noreturn]] void expect_binary_operator();
	[[noreturn]] void end_of_expression();
	[[noreturn]] void divide_by_zero();

	int peek_type() const;
	token next();
	static int precedence(int);

	void skip() {
		if (!tokens.empty()) tokens.pop_back();
	}

	const std::string &name;
	std::vector<token> tokens;
};

int expression_parser::peek_type() const {
	if (tokens.empty()) return token::eof;
	return tokens.back().type;
}

token expression_parser::next() {
	if (tokens.empty()) return token("", token::eof); // error?
	return pop(tokens);
}

void expression_parser::expect_binary_operator() {
	token t = next();

	std::string error;
	error = "### " + name;
	error += " - Expected a binary operator when \"";
	error += t.string;
	error += "\" was encountered.";
	throw std::runtime_error(error);
}

void expression_parser::end_of_expression() {
	std::string error;
	error = "### " + name + " - Unexpected end of expression.";
	throw std::runtime_error(error);
}

void expression_parser::divide_by_zero() {
	std::string error;
	error = "### " + name + " - Attempt to divide by zero.";
	throw std::runtime_error(error);
}


value expression_parser::binary() {
	
	std::vector<value> output;
	std::vector<std::pair<int, int>> operators;

	value v = unary();

	output.emplace_back(std::move(v));
	
	for(;;) {

		// check for an operator.

		int type = peek_type();
		if (type == token::eof) break;
		if (type == ')') break;

		int p = precedence(type);
		if (!p) expect_binary_operator();
		skip();

		while (!operators.empty() && operators.back().second <= p) {
			// reduce top ops.
			int op = operators.back().first;
			operators.pop_back();
			value rhs = pop(output);
			value lhs = pop(output);

			output.emplace_back(eval(op, lhs, rhs));
		}


		operators.push_back(std::make_pair(type, p));
		
		v = unary();

		output.emplace_back(std::move(v));

	}

	// reduce...
	while (!operators.empty()) {

		int op = pop(operators).first;
		value rhs = pop(output);
		value lhs = pop(output);

		output.emplace_back(eval(op, lhs, rhs));
	}

	if (output.size() != 1) throw std::runtime_error("binary stack error");
	return pop(output);
}

int expression_parser::precedence(int op) {
	switch (op) {

		case '*':
		case '%':
		case '/':
			return 3;

		case '+':
		case '-':
			return 4;

		case '>>':
		case '<<':
			return 5;

		case '<':
		case '<=':
		case '>':
		case '>=':
			return 6;

		case '==':
		case '!=':
		case token::equivalent:
		case token::not_equivalent:
			return 7;
		case '&':
			return 8;
		case '^':
			return 9;
		case '|':
			return 10;
		case '&&':
			return 11;
		case '||':
			return 12;
	}
	return 0;
	//throw std::runtime_error("unimplemented op";);
}

value expression_parser::eval(int op, value &lhs, value &rhs) {
	switch (op) {

		case '*':
			return lhs.to_number() * rhs.to_number();

		case '/':
			if (!rhs.to_number()) divide_by_zero();
			return lhs.to_number() / rhs.to_number();

		case '%':
			if (!rhs.to_number()) divide_by_zero();
			return lhs.to_number() % rhs.to_number();


		case '+':
			return lhs.to_number() + rhs.to_number();
		case '-':
			return lhs.to_number() - rhs.to_number();
		case '>':
			return lhs.to_number() > rhs.to_number();
		case '<':
			return lhs.to_number() < rhs.to_number();

		case '<=':
			return lhs.to_number() <= rhs.to_number();

		case '>=':
			return lhs.to_number() >= rhs.to_number();

		case '>>':
			return lhs.to_number() >> rhs.to_number();

		case '<<':
			return lhs.to_number() >> rhs.to_number();

			// logical || . NaN ok
		case '||':
			return lhs.to_number(1) || rhs.to_number(1);

			// logical && . NaN ok
		case '&&':
			return lhs.to_number(1) && rhs.to_number(1);

		case '|':
			return lhs.to_number() | rhs.to_number();

		case '&':
			return lhs.to_number() & rhs.to_number();

		case '^':
			return lhs.to_number() ^ rhs.to_number();

		case '==':
			// string ==.  0x00==0   -> 0
			// as a special case, 	0=="".  go figure.
			if (lhs.string == "" && rhs.string == "0") return 1;
			if (lhs.string == "0" && rhs.string == "") return 1;
			return lhs.string == rhs.string;

		case '!=':
			if (lhs.string == "" && rhs.string == "0") return 0;
			if (lhs.string == "0" && rhs.string == "") return 0;
			return lhs.string != rhs.string;


	}
	// todo...
	throw std::runtime_error("unimplemented op");
}

value expression_parser::unary() {

	int type = peek_type();

	switch (type) {
		case '-':
		case '+':
		case '!':
		case '~':
			next();
			value v = unary();
			// + is a nop.. doesn't even check if it's a number.
			if (type == '-') v = -v.to_number();
			if (type == '~') v = ~v.to_number();
			if (type == '!') v = !v.to_number(1); // logical !, NaN ok.

			return v;
	}

	return terminal();
}

value expression_parser::terminal() {

	int type = peek_type();

	if (type == token::text) {
		token t = next();
		return value(std::move(t.string));
	}

	if (type == '(') {
		next();
		value v = binary();
		type = peek_type();
		if (type != ')') {
			end_of_expression();
		}
		next();
		return v;
	}
	// insert a fake token.
	return value();
}

int32_t expression_parser::evaluate() {
	if (tokens.empty()) return 0;

	value v = binary();
	if (!tokens.empty()) {
		if (tokens.back().type == ')')
			throw std::runtime_error("### MPW Shell - Extra ) command.");
		throw std::runtime_error("evaluation stack error."); // ?? should be caught above.
	}
	return v.to_number(1);
}

int32_t evaluate_expression(const std::string &name, std::vector<token> &&tokens) {

	expression_parser p(name, std::move(tokens));
	return p.evaluate();
}
