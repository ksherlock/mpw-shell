#ifndef __builtins_h__
#define __builtins_h__

#include <vector>
#include <string>

class Environment;
class fdmask;
class token;

int builtin_directory(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_echo(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_parameters(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_quote(Environment &e, const std::vector<std::string> &tokens, const fdmask &);
int builtin_set(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_unset(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_export(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_unexport(Environment &e, const std::vector<std::string> &, const fdmask &);
int builtin_which(Environment &e, const std::vector<std::string> &, const fdmask &);

int builtin_evaluate(Environment &e, std::vector<token> &&, const fdmask &);

#endif