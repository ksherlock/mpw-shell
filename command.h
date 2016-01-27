#include <memory>
#include <vector>
#include <array>

typedef std::unique_ptr<struct command> command_ptr;
typedef std::vector<command_ptr> command_ptr_vector;
typedef std::array<command_ptr, 2> command_ptr_pair;

struct command {
	enum {

	};
	int type;
	virtual ~command();
	virtual int run();
};

struct simple_command  : public command {
	std::string text;
};

struct binary_command : public command {
	command_ptr_pair children;
};

struct or_command : public binary_command {

};

struct and_command : public binary_command {

};

struct begin_command : public command {
	command_ptr_vector children;
	std::string end; 
};

struct if_command : public command {
	std::string begin;
	command_ptr_vector children;
	command_ptr_vector else_clause;
	std::string end;
};
