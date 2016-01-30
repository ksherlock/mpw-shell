#ifndef __lemon_base_h__
#define __lemon_base_h__
#include <cstdio>

template<class TokenType>
class lemon_base {
public:
	typedef TokenType token_type;
	
	virtual ~lemon_base() = default;

	#if 0
	virtual typename std::enable_if<std::is_copy_constructible<TokenType>::value, void>::type 
	parse(int yymajor, const TokenType &yyminor) = 0;
	#endif

	//virtual typename std::enable_if<std::is_move_constructible<TokenType>::value, void>::type 
	virtual void parse(int yymajor, TokenType &&yyminor) = 0;
    virtual void trace(FILE *, const char *) = 0;

protected:
	virtual void parse_accept() {}
	virtual void parse_failure() {}
	virtual void stack_overflow() {}
	virtual void syntax_error(int yymajor, TokenType &yyminor) {}
	lemon_base() {}

private:
	lemon_base(const lemon_base &) = delete;
	lemon_base(lemon_base &&) = delete;
	lemon_base &operator=(const lemon_base &) = delete;
	lemon_base &operator=(lemon_base &&) = delete;
};

#endif