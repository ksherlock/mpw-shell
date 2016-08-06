#include "environment.h"
#include <cstdio>
#include <cstdarg>

#include <algorithm>

#include "error.h"

namespace {


	std::string &lowercase(std::string &s) {
		std::transform(s.begin(), s.end(), s.begin(), [](char c){ return std::tolower(c); });
		return s;
	}

	/* 0 or "" -> false.  all others -> true */
	bool tf(const std::string &s) {
		if (s.empty()) return false;
		if (s.size() == 1 && s == "0") return false;
		return true;
	}

	bool tf(long v) { return v; }

	// used for #.  base 10 only, extra chars ignored.
	int to_pound_int(const std::string &s) {
		if (s.empty()) return 0;
		try {
			int n = stoi(s);
			return std::max(n, (int)0);
		}
		catch(std::exception e) {}
		return 0;
	}

	int to_pound_int(long n) { return std::max(n, (long)0); }

}

	std::string Environment::get(const std::string & key) const {
		auto iter = find(key);
		if (iter == end()) return "";
		return iter->second;
	}

	Environment::iterator Environment::find( const std::string & key ) {
		std::string k(key);
		lowercase(k);
		return _table.find(k);
	}

	Environment::const_iterator Environment::find( const std::string & key ) const {
		std::string k(key);
		lowercase(k);
		return _table.find(k);
	}


	void Environment::set(const std::string &key, const std::string &value, bool exported) {
		std::string k(key);
		lowercase(k);

		if (k == "echo") _echo = tf(value);
		if (k == "exit") _exit = tf(value);
		if (k == "test") _test = tf(value);
		if (k == "#") _pound = to_pound_int(value);

		// don't need to check {status} because that will be clobbered
		// by the return value.
		set_common(k, value, exported);
	}

	void Environment::set(const std::string &key, long value, bool exported) {
		std::string k(key);
		lowercase(k);

		if (k == "echo") _echo = tf(value);
		if (k == "exit") _exit = tf(value);
		if (k == "test") _test = tf(value);
		if (k == "#") _pound = to_pound_int(value);

		// don't need to check {status} because that will be clobbered
		// by the return value.
		set_common(k, std::to_string(value), exported);
	}

	void Environment::set_argv(const std::string &argv0, const std::vector<std::string>& argv) {
		set_common("0", argv0, false);
		set_argv(argv);
	}
	void Environment::set_argv(const std::vector<std::string>& argv) {
		_pound = argv.size();
		set_common("#", std::to_string(argv.size()), false);

		int n = 1;
		for (const auto &s : argv) {
			set_common(std::to_string(n++), s, false);
		}

		// parameters, "parameters" ...
		std::string p;
		for (const auto &s : argv) {
			p.push_back('"');
			p += s;
			p.push_back('"');
			p.push_back(' ');
		}
		p.pop_back();
		set_common("\"parameters\"", p, false);
		p.clear();

		for (const auto &s : argv) {
			p += s;
			p.push_back(' ');
		}
		p.pop_back();
		set_common("parameters", p, false);


	}


	void Environment::set_common(const std::string &k, const std::string &value, bool exported)
	{
		EnvironmentEntry v(value, exported);

		auto iter = _table.find(k);
		if (iter == _table.end()) {
			_table.emplace(std::make_pair(k, std::move(v)));
		}
		else {
			// if previously exported, keep exported.
			if (iter->second) v = true;
			iter->second = std::move(v);
		}	
	}





	void Environment::unset(const std::string &key) {
		std::string k(key);
		lowercase(k);
		if (k == "echo") _echo = false;
		if (k == "exit") _exit = false;
		if (k == "test") _test = false;
		if (k == "#") _pound = 0;
		_table.erase(k);
	}

	void Environment::unset() {
		_table.clear();
		_echo = false;
		_exit = false;
		_test = false;
		_status = 0;	
	}


	int Environment::status(int i, const std::nothrow_t &) {

		if (_status == i) return i;

		_status = i;
		_table["status"] = std::to_string(i);
		return i;
	}

	int Environment::status(int i, bool throwup) {
		status(i, std::nothrow);

		if (throwup && _exit && i) {
			throw execution_of_input_terminated(i);
		}
		return i;
	}

	void Environment::echo(const char *fmt, ...) const {
		if (_echo && !_startup) {
			for (int i = 0; i <= _indent; ++i) {
				fputc(' ', stderr);
				fputc(' ', stderr);
			}
			va_list ap;
			va_start(ap, fmt);
			va_end(ap);
			vfprintf(stderr, fmt, ap);
			fputc('\n', stderr);
		}
	}


	void Environment::rebuild_aliases() {
		std::string as;
		for (const auto &p : _alias_table) {
			as += p.first;
			as.push_back(',');
		}
		as.pop_back();
		set_common("aliases", as, true);
	}

	void Environment::remove_alias() {
		_alias_table.clear();
		set_common("aliases", "", true);
	}

	void Environment::remove_alias(const std::string &name) {

		std::string k(name);
		lowercase(k);

		auto iter = std::remove_if(_alias_table.begin(), _alias_table.end(), [&k](const auto &p){
			return k == p.first;
		});
		_alias_table.erase(iter, _alias_table.end());
		rebuild_aliases();
	}

	const std::string &Environment::find_alias(const std::string &name) const {

		std::string k(name);
		lowercase(k);

		auto iter = std::find_if(_alias_table.begin(), _alias_table.end(), [&k](const auto &p){
			return k == p.first;
		});

		if (iter == _alias_table.end()) {
			static std::string empty;
			return empty;
		}
		return iter->second;
	}

	void Environment::add_alias(std::string &&name, std::string &&value) {

		lowercase(name);

		auto iter = std::find_if(_alias_table.begin(), _alias_table.end(), [&name](const auto &p){
			return name == p.first;
		});

		if (iter == _alias_table.end()) {
			_alias_table.emplace_back(std::make_pair(std::move(name), std::move(value)));
		} else {
			iter->second = std::move(value);
		}
		rebuild_aliases();
	}





