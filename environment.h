#ifndef __environment_h__
#define __environment_h__

#include <string>
#include <unordered_map>
#include <utility>

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
	typedef std::unordered_map<std::string, EnvironmentEntry> mapped_type;
	typedef mapped_type::iterator iterator;
	typedef mapped_type::const_iterator const_iterator;

	//const EnvironmentEntry & lookup(const std::string &s);

	void set(const std::string &k, const std::string &value, bool exported = false);
	void unset(const std::string &k);
	void unset();

	constexpr bool echo() const noexcept { return _echo; }
	constexpr bool test() const noexcept { return _test; }
	constexpr bool exit() const noexcept { return _and_or ? false : _exit; }
	constexpr int status() const noexcept { return _status; }

	bool and_or(bool v) { std::swap(v, _and_or); return v; }

	int status(int i);

	constexpr bool startup() const noexcept { return _startup; }
	constexpr void startup(bool tf) noexcept { _startup = tf; }


	constexpr bool passthrough() const noexcept { return _passthrough; }
	constexpr void passthrough(bool tf) noexcept { _passthrough = tf; }

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


	void echo(const char *format, ...);

private:
	// magic variables.

	bool _exit = false;
	bool _test = false;

	bool _and_or = false;

	bool _echo = false;
	int _status = 0;
	int _indent = 0;
	bool _startup = false;
	bool _passthrough = false;

	std::unordered_map<std::string, EnvironmentEntry> _table;
};


#endif
