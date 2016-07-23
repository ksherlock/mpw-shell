

#include <string>
#include <algorithm>
#include <stdint.h>

uint8_t unicode_to_macroman(uint16_t c){
	

	if (c < 0x80) return c;

#undef _
#define _(macroman,unicode,comment) if (c == unicode) return macroman;
#include "macroman.x"

#undef _
	return 0;
}


uint16_t macroman_to_unicode(uint8_t c) {

	static uint16_t table[] = {
#undef _
#define _(macroman,unicode,comment) unicode ,
#include "macroman.x"
#undef _
	};

	if (c < 0x80) return c;
	return table[c - 0x80];

}





std::string utf8_to_macroman(const std::string &s) {
	

	if (std::find_if(s.begin(), s.end(), [](unsigned char c){ return c & 0x80; }) == s.end())
		return s;

	std::string rv;
	rv.reserve(s.size());

	unsigned cs = 0;
	uint16_t tmp;

	for (unsigned char c : s) {
		switch(cs) {
		case 0:
			if (c <= 0x7f) {
				rv.push_back(c);
				continue;
			}
			if ((c & 0b11100000) == 0b11000000) {
				(tmp = c & 0b00011111);
				cs = 1;
				continue;
			}
			if ((c & 0b11110000) == 0b11100000) {
				(tmp = c & 0b00001111);
				cs = 2;
				continue;
			}
			if ((c & 0b11111000) == 0b11110000) {
				(tmp = c & 0b00000111);
				cs = 3;
				continue;
			}
			// not utf8...
			break;
		case 1:
		case 2:
		case 3:
			if ((c & 0b11000000) != 0b10000000) {
				//not utf8...
			}
			tmp = (tmp << 6) + (c & 0b00111111);
			if(--cs == 0) {
				c = unicode_to_macroman(tmp);
				if (c) rv.push_back(c);
			}
			break;
		}
	}

	return rv;
}


std::string macroman_to_utf8(const std::string &s) {

	if (std::find_if(s.begin(), s.end(), [](unsigned char c){ return c & 0x80; }) == s.end())
		return s;

	std::string rv;
	rv.reserve(s.size());

	for (uint8_t c : s) {
		if (c <= 0x7f) { rv.push_back(c); continue; }

		uint16_t tmp = macroman_to_unicode(c);

		if (tmp <= 0x7f) {
			rv.push_back(tmp);
		} else if (tmp <= 0x07ff) {
			uint8_t a,b;
			b = tmp & 0b00111111; tmp >>= 6;
			a = tmp;
			rv.push_back(0b11000000 | a);
			rv.push_back(0b10000000 | b);
		} else { // tmp <= 0xffff
			uint8_t a,b,c;
			c = tmp & 0b00111111; tmp >>= 6;
			b = tmp & 0b00111111; tmp >>= 6;
			a = tmp;
			rv.push_back(0b11100000 | a);
			rv.push_back(0b10000000 | b);
			rv.push_back(0b10000000 | c);
		}
	}
	return rv;
}


