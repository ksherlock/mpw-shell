#ifndef __environment_h__
#define __environment_h__

#include <map>
#include <new>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>



// environment has a bool which indicates if exported.
struct EnvironmentEntry {
public:
	operator bool() const { return exported; }
	operator bool&() { return exported; }

	operator const std::string&() const { return value; }
	operator std::string&() { return value; }

	const char *c_str() const { return value.c_str(); }

	EnvironmentEntry() = default;
	EnvironmentEntry(const EnvironmentEntry &) = default;
	EnvironmentEntry(EnvironmentEntry &&) = default;

	EnvironmentEntry(const std::string &s, bool e = false) : value(s), exported(e)
	{}
	EnvironmentEntry(std::string &&s, bool e = false) : value(std::move(s)), exported(e)
	{}

	~EnvironmentEntry() = default;

	EnvironmentEntry& operator=(bool rhs) { exported = rhs; return *this; }
	EnvironmentEntry& operator=(const std::string &rhs) { value = rhs; return *this; }
	EnvironmentEntry& operator=(const EnvironmentEntry &) = default;
	EnvironmentEntry& operator=(EnvironmentEntry &&) = default;

private:
	std::string value;
	bool exported = false;

};



class Environment {

public:
	typedef std::map<std::string, EnvironmentEntry> mapped_type;
	typedef mapped_type::iterator iterator;
	typedef mapped_type::const_iterator const_iterator;


	typedef std::vector<std::pair<std::string, std::string>> alias_table_type;
	typedef alias_table_type::const_iterator const_alias_iterator;

	//const EnvironmentEntry & lookup(const std::string &s);

	void set_argv(const std::string &argv0, const std::vector<std::string>& argv);
	void set_argv(const std::vector<std::string>& argv);

	void set(const std::string &k, const std::string &value, bool exported = false);
	void set(const std::string &k, long l, bool exported = false);
	void unset(const std::string &k);
	void unset();

	std::string get(const std::string &k) const;

	bool echo() const noexcept { return _echo; }
	bool test() const noexcept { return _test; }
	bool exit() const noexcept { return _exit; }
	int status() const noexcept { return _status; }
	int pound() const noexcept { return _pound; }

	int status(int i, bool throw_up = true);
	int status(int i, const std::nothrow_t &);

	bool startup() const noexcept { return _startup; }
	void startup(bool tf) noexcept { _startup = tf; }


	bool passthrough() const noexcept { return _passthrough; }
	void passthrough(bool tf) noexcept { _passthrough = tf; }

	template<class FX>
	void foreach(FX && fx) { for (const auto &kv : _table) { fx(kv.first, kv.second); }}

	iterator begin() { return _table.begin(); }
	const_iterator begin() const { return _table.begin(); }
	const_iterator cbegin() const { return _table.cbegin(); }

	iterator end() { return _table.end(); }
	const_iterator end() const { return _table.end(); }
	const_iterator cend() const { return _table.cend(); }


	iterator find( const std::string & key );
	const_iterator find( const std::string & key ) const;


	void echo(const char *format, ...) const;

	template<class FX>
	void indent_and(FX &&fx) {
		int i = _indent++;
		try { fx(); _indent = i; }
		catch (...) { _indent = i; throw; } 
	}

	template<class FX>
	void loop_indent_and(FX &&fx) {
		int i = _indent++;
		int j = _loop++;
		try { fx(); _indent = i; _loop = j; }
		catch (...) { _indent = i; _loop = j; throw; } 
	}

	bool loop() const noexcept { return _loop; }

	const alias_table_type &aliases() const { return _alias_table; }

	void add_alias(std::string &&name, std::string &&value);
	const std::string &find_alias(const std::string &s) const;

	void remove_alias(const std::string &name);
	void remove_alias();

	const_alias_iterator alias_begin() const { return _alias_table.begin(); }
	const_alias_iterator alias_end() const { return _alias_table.end(); }

private:
	// magic variables.

	friend class indent_helper;

	int _indent = 0;
	int _loop = 0;

	bool _exit = false;
	bool _test = false;

	bool _echo = false;
	int _status = 0;
	int _pound = 0;
	bool _startup = false;
	bool _passthrough = false;

	void set_common(const std::string &, const std::string &, bool);
	void rebuild_aliases();

	mapped_type _table;

	alias_table_type _alias_table;
};

/*
class indent_helper {
public:
	indent_helper(Environment &e) : env(e) { env._indent++; }
	void release() { if (active) { active = false; env._indent--; }}
	~indent_helper() { if (active) env._indent--; }
private:
	Environment &env;
	bool active = true;
};
*/

#endif
