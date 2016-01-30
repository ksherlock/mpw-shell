#ifndef __command_h__
#define __command_h__

#include <memory>
#include <vector>
#include <array>
#include <string>
#include "phase2-parser.h"

typedef std::unique_ptr<struct command> command_ptr;
typedef std::vector<command_ptr> command_ptr_vector;
typedef std::array<command_ptr, 2> command_ptr_pair;


struct command {

	command(int t = 0) : type(t)
	{}

	int type = 0;
	virtual ~command();
	virtual int execute() = 0;
};

struct simple_command  : public command {
	template<class S>
	simple_command(S &&s) : command(COMMAND), text(std::forward<S>(s))
	{}

	std::string text;

	virtual int execute() final override;
};

struct binary_command : public command {

	binary_command(int t, command_ptr &&a, command_ptr &&b) :
		command(t), children({{std::move(a), std::move(b)}})
	{}

	command_ptr_pair children;

};

struct or_command : public binary_command {
	or_command(command_ptr &&a, command_ptr &&b) :
		binary_command(PIPE_PIPE, std::move(a), std::move(b)) 
	{}

	virtual int execute() final override;
};

struct and_command : public binary_command {
	and_command(command_ptr &&a, command_ptr &&b) :
		binary_command(AMP_AMP, std::move(a), std::move(b)) 
	{}

	virtual int execute() final override;
};


#if 0
struct pipe_command : public binary_command {
	and_command(command_ptr &&a, command_ptr &&b) :
		binary_command(PIPE, std::move(a), std::move(b)) 
	{}

	virtual int execute() final override;
};
#endif


struct vector_command : public command {
	vector_command(int t, command_ptr_vector &&v) : command(t), children(std::move(v))
	{}

	command_ptr_vector children;
	virtual int execute() override;
};

struct begin_command : public vector_command {
	template<class S>
	begin_command(int t, command_ptr_vector &&v, S &&s) :
		vector_command(t, std::move(v)), end(std::forward<S>(s))
	{}

	std::string end; 

	virtual int execute() final override;
};

typedef std::unique_ptr<struct if_else_clause> if_else_clause_ptr;
struct if_command : public command {

	typedef std::vector<if_else_clause_ptr> clause_vector_type;

	template<class S>
	if_command(clause_vector_type &&v, S &&s) :
		command(IF), clauses(std::move(v)), end(std::forward<S>(s))
	{}


	clause_vector_type clauses;
	std::string end;

	virtual int execute() final override;
};

struct if_else_clause : public vector_command {

	template<class S>
	if_else_clause(int t, command_ptr_vector &&v, S &&s) :
		vector_command(t, std::move(v)), clause(std::forward<S>(s))
	{}

	std::string clause;

	bool evaluate();
	//virtual int execute() final override;
};

#endif