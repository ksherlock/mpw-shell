
#line 1 "mpw-shell-commands.rl"
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>

#include <stdio.h>

typedef std::shared_ptr<command> command_ptr;
typedef std::weak_ptr<command> weak_command_ptr;

class command {
	enum type {
		command_if = 1,
		command_else,
		command_else_if,
		command_end
	} = 0;
	std::string line;
	command_ptr next;
	weak_command_ptr alternate; // if -> else -> end. 
};



#line 49 "mpw-shell-commands.rl"



int classify(const std::string &line) {

	
#line 35 "mpw-shell-commands.c"
static const int classify_start = 8;
static const int classify_first_final = 8;
static const int classify_error = 0;

static const int classify_en_main = 8;


#line 55 "mpw-shell-commands.rl"

	int cs;
	const unsigned char *p = (const unsigned char *)line.data();
	const unsigned char *pe = (const unsigned char *)line.data() + line.size();
	const unsigned char *eof = pe;
	const unsigned char *te, *ts;

	
#line 52 "mpw-shell-commands.c"
	{
	cs = classify_start;
	ts = 0;
	te = 0;
	act = 0;
	}

#line 63 "mpw-shell-commands.rl"

	
#line 63 "mpw-shell-commands.c"
	{
	if ( p == pe )
		goto _test_eof;
	switch ( cs )
	{
tr5:
#line 40 "mpw-shell-commands.rl"
	{{p = ((te))-1;}{ return command_else; }}
	goto st8;
tr13:
#line 39 "mpw-shell-commands.rl"
	{ return command_else;}
#line 39 "mpw-shell-commands.rl"
	{te = p;p--;}
	goto st8;
tr14:
#line 39 "mpw-shell-commands.rl"
	{te = p;p--;}
	goto st8;
tr16:
#line 40 "mpw-shell-commands.rl"
	{te = p;p--;{ return command_else; }}
	goto st8;
tr17:
#line 42 "mpw-shell-commands.rl"
	{ return command_else_if; }
#line 42 "mpw-shell-commands.rl"
	{te = p;p--;}
	goto st8;
tr18:
#line 42 "mpw-shell-commands.rl"
	{te = p;p--;}
	goto st8;
tr19:
#line 43 "mpw-shell-commands.rl"
	{te = p+1;{return command_else_if; }}
	goto st8;
tr20:
#line 45 "mpw-shell-commands.rl"
	{ return command_end; }
#line 45 "mpw-shell-commands.rl"
	{te = p;p--;}
	goto st8;
tr21:
#line 45 "mpw-shell-commands.rl"
	{te = p;p--;}
	goto st8;
tr22:
#line 46 "mpw-shell-commands.rl"
	{te = p+1;{return command_end; }}
	goto st8;
tr23:
#line 36 "mpw-shell-commands.rl"
	{ return command_if; }
#line 36 "mpw-shell-commands.rl"
	{te = p;p--;}
	goto st8;
tr24:
#line 36 "mpw-shell-commands.rl"
	{te = p;p--;}
	goto st8;
tr25:
#line 37 "mpw-shell-commands.rl"
	{te = p+1;{return command_if; }}
	goto st8;
st8:
#line 1 "NONE"
	{ts = 0;}
	if ( ++p == pe )
		goto _test_eof8;
case 8:
#line 1 "NONE"
	{ts = p;}
#line 137 "mpw-shell-commands.c"
	switch( (*p) ) {
		case 69u: goto st1;
		case 73u: goto st7;
		case 101u: goto st1;
		case 105u: goto st7;
	}
	goto st0;
st0:
cs = 0;
	goto _out;
st1:
	if ( ++p == pe )
		goto _test_eof1;
case 1:
	switch( (*p) ) {
		case 76u: goto st2;
		case 78u: goto st6;
		case 108u: goto st2;
		case 110u: goto st6;
	}
	goto st0;
st2:
	if ( ++p == pe )
		goto _test_eof2;
case 2:
	switch( (*p) ) {
		case 83u: goto st3;
		case 115u: goto st3;
	}
	goto st0;
st3:
	if ( ++p == pe )
		goto _test_eof3;
case 3:
	switch( (*p) ) {
		case 69u: goto st9;
		case 101u: goto st9;
	}
	goto st0;
st9:
	if ( ++p == pe )
		goto _test_eof9;
case 9:
	switch( (*p) ) {
		case 9u: goto tr15;
		case 32u: goto tr15;
	}
	goto tr14;
tr15:
#line 1 "NONE"
	{te = p+1;}
	goto st10;
st10:
	if ( ++p == pe )
		goto _test_eof10;
case 10:
#line 194 "mpw-shell-commands.c"
	switch( (*p) ) {
		case 9u: goto st4;
		case 32u: goto st4;
		case 73u: goto st5;
		case 105u: goto st5;
	}
	goto tr16;
st4:
	if ( ++p == pe )
		goto _test_eof4;
case 4:
	switch( (*p) ) {
		case 9u: goto st4;
		case 32u: goto st4;
		case 73u: goto st5;
		case 105u: goto st5;
	}
	goto tr5;
st5:
	if ( ++p == pe )
		goto _test_eof5;
case 5:
	switch( (*p) ) {
		case 70u: goto st11;
		case 102u: goto st11;
	}
	goto tr5;
st11:
	if ( ++p == pe )
		goto _test_eof11;
case 11:
	switch( (*p) ) {
		case 9u: goto tr19;
		case 32u: goto tr19;
	}
	goto tr18;
st6:
	if ( ++p == pe )
		goto _test_eof6;
case 6:
	switch( (*p) ) {
		case 68u: goto st12;
		case 100u: goto st12;
	}
	goto st0;
st12:
	if ( ++p == pe )
		goto _test_eof12;
case 12:
	switch( (*p) ) {
		case 9u: goto tr22;
		case 32u: goto tr22;
	}
	goto tr21;
st7:
	if ( ++p == pe )
		goto _test_eof7;
case 7:
	switch( (*p) ) {
		case 70u: goto st13;
		case 102u: goto st13;
	}
	goto st0;
st13:
	if ( ++p == pe )
		goto _test_eof13;
case 13:
	switch( (*p) ) {
		case 9u: goto tr25;
		case 32u: goto tr25;
	}
	goto tr24;
	}
	_test_eof8: cs = 8; goto _test_eof; 
	_test_eof1: cs = 1; goto _test_eof; 
	_test_eof2: cs = 2; goto _test_eof; 
	_test_eof3: cs = 3; goto _test_eof; 
	_test_eof9: cs = 9; goto _test_eof; 
	_test_eof10: cs = 10; goto _test_eof; 
	_test_eof4: cs = 4; goto _test_eof; 
	_test_eof5: cs = 5; goto _test_eof; 
	_test_eof11: cs = 11; goto _test_eof; 
	_test_eof6: cs = 6; goto _test_eof; 
	_test_eof12: cs = 12; goto _test_eof; 
	_test_eof7: cs = 7; goto _test_eof; 
	_test_eof13: cs = 13; goto _test_eof; 

	_test_eof: {}
	if ( p == eof )
	{
	switch ( cs ) {
	case 9: goto tr13;
	case 10: goto tr16;
	case 4: goto tr5;
	case 5: goto tr5;
	case 11: goto tr17;
	case 12: goto tr20;
	case 13: goto tr23;
	}
	}

	_out: {}
	}

#line 65 "mpw-shell-commands.rl"

	return 0;	
}
