

#include <string>
#include <algorithm>
#include <stdint.h>
#include <array>


struct mu_pair {
	uint16_t unicode;
	uint8_t macroman;

	constexpr bool operator<(const mu_pair &rhs) const noexcept {
		return unicode < rhs.unicode;
	}

	constexpr bool operator==(const mu_pair &rhs) const noexcept {
		return unicode == rhs.unicode;
	}

	constexpr bool operator<(uint16_t c) const noexcept {
		return unicode < c;
	}

	constexpr bool operator==(uint16_t c) const noexcept {
		return unicode == c;
	}
};

uint8_t unicode_to_macroman(uint16_t c){


	static std::array< mu_pair, 0x80> table = {{
#undef _
#define _(macroman, unicode, comment)  mu_pair{ unicode, macroman } ,
#include "macroman.x"
#undef _
	}};

	static bool init = false;

	if (c < 0x80) return c;

	if (!init) {
		init = true;
		std::sort(table.begin(), table.end());
		init = true;
	}

	auto iter = std::lower_bound(table.begin(), table.end(), c);
	if (iter != table.end() && iter->unicode == c) return iter->macroman;
	return 0;
}


uint16_t macroman_to_unicode(uint8_t c) {

	static std::array<uint16_t, 0x80> table = {{
#undef _
#define _(macroman,unicode,comment) unicode ,
#include "macroman.x"
#undef _
	}};

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


