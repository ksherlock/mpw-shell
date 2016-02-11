#include "environment.h"
#include <cstdio>
#include <cstdarg>

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

		// don't need to check {status} because that will be clobbered
		// by the return value.

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

	void Environment::echo(const char *fmt, ...) {
		if (_echo && !_startup) {
			for (unsigned i = 0; i < _indent; ++i) {
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


