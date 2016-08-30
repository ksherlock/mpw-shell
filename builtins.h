#ifndef __builtins_h__
#define __builtins_h__

#include <vector>
#include <string>

class Environment;
class fdmask;
class token;

int builtin_aboutbox(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_catenate(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_directory(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_echo(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_exists(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_export(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_parameters(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_quote(Environment &e, const std::vector<std::string> &tokens, const fdmask &);
int builtin_set(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_shift(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_unexport(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_unset(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_version(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_which(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_alias(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_unalias(Environment &e, const std::vector<std::string> &, const fdmask &);

int builtin_execute(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_true(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_false(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_quit(Environment &e, const std::vector<std::string> &, const fdmask &);


int builtin_evaluate(Environment &e, std::vector<token> &&, const fdmask &);

#endif