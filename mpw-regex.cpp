
#include "mpw-regex.h"
#include "environment.h"

typedef std::string::const_iterator iterator;

namespace {
	bool ecma_special(unsigned char c) {
		//
		switch(c) {
		case '|':
		case '{':
		case '}':
		case '(':
		case ')':
		case '[':
		case ']':
		case '*':
		case '+':
		case '^':
		case '$':
		case '.':
		case '\\':
			return true;
		default:
			return false;
		}
	}
}

mpw_regex::mpw_regex(const std::string &s, bool slash) {
	convert_re(s, slash);
}

bool mpw_regex::is_glob(const std::string &s) {
	bool esc = false;
	for (unsigned char c : s) {
		if (esc) {
			esc = false;
			continue;
		}
		switch(c) {
		case 0xb6:
			esc = true;
			break;
		case '[':
		case '?':
		case '*':
		case '+':
		case 0xc7:
		case 0xc5:
			return true;
		default:
			break;
		}
	}
	return false;
}

bool mpw_regex::match(const std::string &s, Environment &e) {
	 std::smatch m;
	bool ok = std::regex_match(s, m, re);
	if (!ok) return false;

	for (int i = 0; i < 10; ++i) {
		int index = capture_map[i];

		if (index && index < m.size() && m[index].matched) {
			std::string v(m[index].first, m[index].second);
			std::string k("\xa8");
			k += (i + '0');
			e.set(k, std::move(v));
		}
	}

	return true;
}

bool mpw_regex::match(const std::string &s) {
	return std::regex_match(s, re);
}


// convert a mpw-flavor regex to std::regex flavor regex.
void mpw_regex::convert_re(const std::string &s, bool slash) {


	std::string accumulator;
	auto iter = s.begin();
	auto end = s.end();

	if (slash) {
		if (iter == end || *iter++ != '/')
			throw std::regex_error(std::regex_constants::error_space);
	}

	iter = convert_re(iter, end, accumulator, slash ? '/' : 0);

	if (iter != end) throw std::regex_error(std::regex_constants::error_space);


	re = std::regex(accumulator);
	if (slash) key = s;
	else key = "/" + s + "/";
}


iterator mpw_regex::convert_re(iterator iter, iterator end, std::string &accumulator, unsigned char term) {

	while (iter != end) {
		unsigned char c = *iter++;
		if (c == 0xb6) {
			// escape
			if (iter == end) throw std::regex_error(std::regex_constants::error_escape);
			c = *iter++;
			if (ecma_special(c))
				accumulator += '\\';
			accumulator += c;
			continue;
		}
		if (term && c == term) {
			return iter;
		}
		if (c == '?') {
			// match any char
			accumulator += '.';
			continue;
		}
		if (c == 0xc5) {
			// match any string
			accumulator += ".*";
			continue;
		}
		if (c == '[') {
			// begin a set
			iter = convert_re_set(iter, end, accumulator);
			continue;
		}
		if (c == '(') {
			// begin a capture
			iter = convert_re_capture(iter, end, accumulator);
			continue;
		}
		if (c == 0xc7) {
			// repeat
			iter = convert_re_repeat(iter, end, accumulator);
			continue;
		}
		if (c == '+' || c == '*') {
			// same meaning
			accumulator += c;
			continue;
		}
		if (ecma_special(c)) {
			accumulator += '\\';
		}
		accumulator += c;
	}

	if (term) throw std::regex_error(std::regex_constants::error_paren);
	return iter;
}

iterator mpw_regex::convert_re_repeat(iterator iter, iterator end, std::string &accumulator) {
	int min = -1;
	int max = -1;

	accumulator += "{";

	while (iter != end) {
		unsigned char c = *iter++;
		if (c == 0xc8) {
			accumulator += "}";
			return iter;			
		}
		if (c != ',' && !isdigit(c)) break;
		accumulator += c;
	}
	throw std::regex_error(std::regex_constants::error_brace);
}

iterator mpw_regex::convert_re_set(iterator iter, iterator end, std::string &accumulator) {
	// need extra logic to block character classes.

	unsigned char c;
	accumulator += "[";

	if (iter != end && static_cast<unsigned char>(*iter) == 0xc2) {
		accumulator += "^";
		++iter;
	} else if (iter != end && *iter == '^') {
		// leading ^ needs to be escaped.
		accumulator += "\\^";
		++iter;
	}
	while (iter != end) {
		c = *iter++;

		if (c == 0xb6) {
			// escape
			if (iter == end) throw std::regex_error(std::regex_constants::error_escape);
			c = *iter++;
			accumulator += '\\';
			accumulator += c;
			continue;
		}

		if (c == ']') {
			accumulator += "]";
			return iter;
		}
		if (c == '\\') {
			accumulator += "\\\\";
			continue;
		}
		accumulator += c;
	}

	throw std::regex_error(std::regex_constants::error_brack);
}

iterator mpw_regex::convert_re_capture(iterator iter, iterator end, std::string &accumulator) {


	/*
	 * consider: (abc(abc)®1(xyz))®2
	 * m[1] = (abcabcxyz)
	 * m[2] = (abc)
	 * BUT we don't know if it's captured until the ® is parsed.
	 */

	std::string scratch;
	bool capture = false;
	int n = -1;

	int ecma_index = ++num_captures;

	if (iter != end && *iter == '?') {
		// leading ? needs to be escaped.
		scratch += "\\?";
		++iter;
	}
	iter = convert_re(iter, end, scratch, ')');

	// check for capture?
	if (iter != end && static_cast<unsigned char>(*iter) == 0xa8) {
		++iter;
		if (iter == end || !isdigit(*iter))
			throw std::regex_error(std::regex_constants::error_badbrace); // eh
		n = *iter++ - '0';
		capture = true;
	}

	accumulator += '(';
	if (capture) {
		/// ummm capture within a capture? backwards?
		capture_map[n] = ecma_index;
	} else {
		accumulator += "?:";
		// re-number all sub-captures.
		--num_captures;
		for (int &index : capture_map) {
			if (index >= ecma_index) --index;
		} 
	}
	accumulator += scratch;
	accumulator += ')';
	return iter;
}
