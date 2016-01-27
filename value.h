
#ifndef __value_h__
#define __value_h__

#include <string>
#include <cstdint>

// hold a string and number value.

struct value {

public:

	std::string string;
	int32_t number = 0;

	// empty token treated as 0.
	value() : status(valid)
	{}

	value(const value &) = default;
	value(value &&) = default;

	value(int32_t n) :
		string(std::to_string(n)), 
		number(n),
		status(valid)
	{}

	value(const std::string &s) : string(s)
	{}

	value(std::string &&s) : string(std::move(s))
	{}

	value &operator=(const value&) = default;
	value &operator=(value &&) = default;


	int32_t to_number() {
		if (status == unknown)
			scan_number();
		if (status == valid) return number;
		expect_number();
	}

	int32_t to_number(int default_value) noexcept {
		if (status == unknown)
			scan_number();
		if (status == valid) return number;
		return default_value;
	}

	bool is_number() noexcept {
		if (status == unknown)
			scan_number();
		return status == valid;
	}


private:
	[[noreturn]] void expect_number() const;
	void scan_number() noexcept;

	mutable enum  {
		unknown,
		valid,
		invalid
	} status = unknown;

};

#endif
