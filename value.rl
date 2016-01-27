
#include "value.h"
#include <stdexcept>

void value::expect_number() const {

	std::string error;

	error =  "Expected a number when \"";
	error += string;
	error += "\" was encountered";

	throw std::domain_error(error);
}

void value::scan_number(void) noexcept {

%%{
		machine scanner;
		hexnumber = 
			('$' | '0x' | '0X')
			(
				[0-9] ${ value = (value << 4) + fc - '0'; }
				|
				[A-Fa-f] ${value = (value << 4) + (fc | 0x20) - 'a' + 10; }
			)+
			;

		binnumber =
			('0b' | '0B')
			[01]+ ${ value = (value << 1) + fc - '0'; }
			;

		octalnumber = 
			'0'
			[0-7]+ ${ value = (value << 3) + fc - '0'; } 
			;

		# a leading 0 is ambiguous since it could also
		# be part of the binary or hex prefix.
		# however, setting it to 0 is safe.
		decnumber =
			'0'
			|
			([1-9] [0-9]*) ${ value = value * 10 + fc - '0'; }
			;


		main :=
			( hexnumber | decnumber |binnumber) 
			%{
				status = valid;
				number = value;
				return;
			}
		;
}%%

	if (string.empty()) {
		// special case.
		status = valid;
		number = 0;
		return;
	}
	const char *p = string.data();
	const char *pe = string.data() + string.size();
	const char *eof = pe;
	int cs;
	int32_t value = 0;

	%%write data;
	%%write init;
	%%write exec;

	status = invalid;
}
