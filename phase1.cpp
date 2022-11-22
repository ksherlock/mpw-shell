#include "phase1.h"
#include <cassert>

enum {
	st_text,
	st_text_esc,

	st_comment,
	st_comment_esc,

	st_vstring,
	st_vstring_esc,

	st_dstring,
	st_dstring_esc,

	st_sstring,
	st_sstring_esc,

	st_estring,
	st_estring1,
	st_estring1_esc,

	st_estring2,
	st_estring2_esc,

	st_estring3,

	st_fstring,
	st_fstring_esc,

	st_bstring,
	st_bstring_esc,

};


int phase1::process(unsigned char c, int st) {

	const unsigned char esc = 0xb6;


	if (c == '\r' || c == '\n') {
		switch (st) {
		case st_text:
		case st_comment:
		default: // will error later.
			flush();
			multiline = false;
			line++;
			return st_text;

		case st_comment_esc:
			multiline = true;
			line++;
			return st_text;

		case st_text_esc:
		case st_vstring_esc:
		case st_dstring_esc:
		case st_sstring_esc:
		case st_estring1_esc:
		case st_estring2_esc:
		case st_fstring_esc:
		case st_bstring_esc:
			multiline = true;
			scratch.pop_back();
			line++;
			return st - 1;
		}
	}

	if (st != st_comment) scratch.push_back(c);

	switch(st) {

		case st_text:
text:
			switch(c) {
				case '#':
					scratch.pop_back();
					return st_comment;
				case esc:
					return st_text_esc;
				case '{':
					return st_vstring;
				case '"':
					return st_dstring;
				case '\'':
					return st_sstring;
				case '`':
					return st_estring;
				case '/':
					return st_fstring;
				case '\\':
					return st_bstring;

				default:
					return st_text;
			}
			break;

		case st_comment:
			if (c == esc) return st_comment_esc;
			return st_comment;
			break;

		case st_comment_esc:
		case st_text_esc:
		case st_dstring_esc:
		case st_estring1_esc:
		case st_estring2_esc:
			return st-1;
			break;


		case st_sstring_esc:
			// fall through
		case st_sstring:
			if (c == '\'') return st_text;
			if (c == esc) return st_sstring_esc;
			return st_sstring;
			break;


		case st_fstring_esc:
			// fall through
		case st_fstring:
			if (c == '/') return st_text;
			if (c == esc) return st_fstring_esc;
			return st_fstring;
			break;

		case st_bstring_esc:
			// fall through
		case st_bstring:
			if (c == '\\') return st_text;
			if (c == esc) return st_bstring_esc;
			return st_bstring;
			break;

		case st_dstring:
			if (c == '\"') return st_text;
			if (c == esc) return st_dstring_esc;
			return st_dstring;
			break;

		case st_vstring_esc:
			// fall through			
		case st_vstring:
			// '{' var '}' or '{{' var '}}'
			// don't care if {{ or { at this point. A single } terminates.
			if (c == '}') return st_text;
			if (c == esc) return st_vstring_esc;
			return st_vstring;

		case st_estring:
			// ``...`` or `...`
			if (c == '`') return st_estring2;
			// fall through.
		case st_estring1:
			if (c == '`') return st_text;
			if (c == esc) return st_estring1_esc;
			return st_estring1;

		case st_estring2:
			if (c == '`') return st_estring3;
			if (c == esc) return st_estring2_esc;
			return st_estring2;

		case st_estring3:
			if (c == '`') return st_text;
			// error! handled later.
			goto text;

			break;

	}
	assert(!"unknown state");
}


void phase1::parse(const unsigned char *begin, const unsigned char *end) {
	while (begin != end) {
		cs = process(*begin++, cs);
	}
}

void phase1::finish() {
	
	cs = process('\n', cs);
	flush();
}

void phase1::reset() {
	cs = st_text;
	multiline = false;
	line = 1;
	scratch.clear();
}

void phase1::flush() {
	multiline = false;
	if (scratch.empty()) return;
	if (_then) _then(std::move(scratch));
	scratch.clear();
}
