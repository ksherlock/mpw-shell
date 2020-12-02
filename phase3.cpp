/*

January, 2016.  Sample class-based version.
#define LEMON_SUPER as the name of a class which overrides lemon_base<TokenType>.  
The parser will be implemented in terms of that.
add a %code section to instantiate it.
 */

/*
** 2000-05-29
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** Driver template for the LEMON parser generator.
**
** The "lemon" program processes an LALR(1) input grammar file, then uses
** this template to construct a parser.  The "lemon" program inserts text
** at each "%%" line.  Also, any "P-a-r-s-e" identifer prefix (without the
** interstitial "-" characters) contained in this template is changed into
** the value of the %name directive from the grammar.  Otherwise, the content
** of this template is copied straight through into the generate parser
** source file.
**
** The following is the concatenation of all %include directives from the
** input grammar file:
*/
#include <cstdio>
#include <cstring>
#include <cassert>
#include <type_traits>
#include <new>
#include <memory>
#include <algorithm>

namespace {

  // use std::allocator etc?


  // this is here so you can do something like Parse(void *, int, my_token &&) or (... const my_token &)
  template<class T> struct yy_fix_type {
    typedef typename std::remove_const<typename std::remove_reference<T>::type>::type type;
  };

  template<>
  struct yy_fix_type<void> {
    typedef struct {} type;
  };

  template<class T, class... Args>
  typename yy_fix_type<T>::type &yy_constructor(void *vp, Args&&... args ) {
    typedef typename yy_fix_type<T>::type TT;
    TT *tmp = ::new(vp) TT(std::forward<Args>(args)...);
    return *tmp;
  }


  template<class T>
  typename yy_fix_type<T>::type &yy_cast(void *vp) {
    typedef typename yy_fix_type<T>::type TT;
    return *(TT *)vp;
  }


  template<class T>
  void yy_destructor(void *vp) {
    typedef typename yy_fix_type<T>::type TT;
    ((TT *)vp)->~TT();
  }


  template<class T>
  void yy_destructor(T &t) {
    t.~T();
  }



  template<class T>
  void yy_move(void *dest, void *src) {
    typedef typename yy_fix_type<T>::type TT;

    TT &tmp = yy_cast<TT>(src);
    yy_constructor<TT>(dest, std::move(tmp));
    yy_destructor(tmp);
  }


  // this is to destruct references in the event of an exception.
  // only the LHS needs to be deleted -- other items remain on the 
  // shift/reduce stack in a valid state 
  // (as long as the destructor) doesn't throw!
  template<class T>
  struct yy_auto_deleter {

    yy_auto_deleter(T &t) : ref(t), enaged(true)
    {}
    yy_auto_deleter(const yy_auto_deleter &) = delete;
    yy_auto_deleter(yy_auto_deleter &&) = delete;
    yy_auto_deleter &operator=(const yy_auto_deleter &) = delete;
    yy_auto_deleter &operator=(yy_auto_deleter &&) = delete;

    ~yy_auto_deleter() {
      if (enaged) yy_destructor(ref);
    }
    void cancel() { enaged = false; }

  private:
    T& ref;
    bool enaged=false;
  };

  template<class T>
  class yy_storage {
  private:
    typedef typename yy_fix_type<T>::type TT;

  public:
    typedef typename std::conditional<
      std::is_trivial<TT>::value,
      TT,
      typename std::aligned_storage<sizeof(TT),alignof(TT)>::type
    >::type type;
  };

}

/************ Begin %include sections from the grammar ************************/
#line 7 "phase3.lemon"


#include "phase3_parser.h"
#include "command.h"
#define LEMON_SUPER phase3
#line 142 "phase3.cpp"
/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols
** in a format understandable to "makeheaders".  This section is blank unless
** "lemon" is run with the "-m" command-line option.
***************** Begin makeheaders token definitions *************************/
/**************** End makeheaders token definitions ***************************/

/* The next sections is a series of control #defines.
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used to store the integer codes
**                       that represent terminal and non-terminal symbols.
**                       "unsigned char" is used if there are fewer than
**                       256 symbols.  Larger types otherwise.
**    YYNOCODE           is a number of type YYCODETYPE that is not used for
**                       any terminal or nonterminal symbol.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       (also known as: "terminal symbols") have fall-back
**                       values which should be used if the original symbol
**                       would not parse.  This permits keywords to sometimes
**                       be used as identifiers, for example.
**    YYACTIONTYPE       is the data type used for "action codes" - numbers
**                       that indicate what to do in response to the next
**                       token.
**    ParseTOKENTYPE     is the data type used for minor type for terminal
**                       symbols.  Background: A "minor type" is a semantic
**                       value associated with a terminal or non-terminal
**                       symbols.  For example, for an "ID" terminal symbol,
**                       the minor type might be the name of the identifier.
**                       Each non-terminal can have a different minor type.
**                       Terminal symbols all have the same minor type, though.
**                       This macros defines the minor type for terminal 
**                       symbols.
**    YYMINORTYPE        is the data type used for all minor types.
**                       This is typically a union of many types, one of
**                       which is ParseTOKENTYPE.  The entry in the union
**                       for terminal symbols is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.  If
**                       zero the stack is dynamically sized using realloc()
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YY_MAX_SHIFT       Maximum value for shift actions
**    YY_MIN_SHIFTREDUCE Minimum value for shift-reduce actions
**    YY_MAX_SHIFTREDUCE Maximum value for shift-reduce actions
**    YY_MIN_REDUCE      Maximum value for reduce actions
**    YY_ERROR_ACTION    The yy_action[] code for syntax error
**    YY_ACCEPT_ACTION   The yy_action[] code for accept
**    YY_NO_ACTION       The yy_action[] code for no-op
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/************* Begin control #defines *****************************************/
#define YYCODETYPE unsigned char
#define YYNOCODE 37
#define YYACTIONTYPE unsigned char
#define ParseTOKENTYPE std::string
typedef union {
  int yyinit;
  yy_storage<ParseTOKENTYPE>::type yy0;
  yy_storage<void>::type yy7;
  yy_storage<command_ptr>::type yy17;
  yy_storage<command_ptr_vector>::type yy35;
  yy_storage<if_command::clause_vector_type>::type yy62;
} YYMINORTYPE;
#ifndef YYSTACKDEPTH
#define YYSTACKDEPTH 100
#endif
#define ParseARG_SDECL
#define ParseARG_PDECL
#define ParseARG_FETCH
#define ParseARG_STORE
#define YYNSTATE             35
#define YYNRULE              36
#define YY_MAX_SHIFT         34
#define YY_MIN_SHIFTREDUCE   61
#define YY_MAX_SHIFTREDUCE   96
#define YY_MIN_REDUCE        97
#define YY_MAX_REDUCE        132
#define YY_ERROR_ACTION      133
#define YY_ACCEPT_ACTION     134
#define YY_NO_ACTION         135
/************* End control #defines *******************************************/
namespace {

/* Define the yytestcase() macro to be a no-op if is not already defined
** otherwise.
**
** Applications can choose to define yytestcase() in the %include section
** to a macro that can assist in verifying code coverage.  For production
** code the yytestcase() macro should be turned off.  But it is useful
** for testing.
*/
#ifndef yytestcase
# define yytestcase(X)
#endif


/* Next are the tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.  
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N <= YY_MAX_SHIFT             Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   N between YY_MIN_SHIFTREDUCE       Shift to an arbitrary state then
**     and YY_MAX_SHIFTREDUCE           reduce by rule N-YY_MIN_SHIFTREDUCE.
**
**   N between YY_MIN_REDUCE            Reduce by rule N-YY_MIN_REDUCE
**     and YY_MAX_REDUCE
**
**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as either:
**
**    (A)   N = yy_action[ yy_shift_ofst[S] + X ]
**    (B)   N = yy_default[S]
**
** The (A) formula is preferred.  The B formula is used instead if:
**    (1)  The yy_shift_ofst[S]+X value is out of range, or
**    (2)  yy_lookahead[yy_shift_ofst[S]+X] is not equal to X, or
**    (3)  yy_shift_ofst[S] equal YY_SHIFT_USE_DFLT.
** (Implementation note: YY_SHIFT_USE_DFLT is chosen so that
** YY_SHIFT_USE_DFLT+X will be out of range for all possible lookaheads X.
** Hence only tests (1) and (2) need to be evaluated.)
**
** The formulas above are for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
**
*********** Begin parsing tables **********************************************/
#define YY_ACTTAB_COUNT (193)
static const YYACTIONTYPE yy_action[] = {
 /*     0 */    86,   87,   66,   67,   68,   69,   70,   71,   12,   98,
 /*    10 */    21,   77,   20,   19,   18,   16,   16,   86,   87,   66,
 /*    20 */    67,   68,   69,   70,   71,   12,   97,   21,   76,   20,
 /*    30 */    19,   18,   86,   87,   66,   67,   68,   69,   70,   71,
 /*    40 */    12,   22,   21,   75,   20,   19,   18,   86,   87,   66,
 /*    50 */    67,   68,   69,   70,   71,   12,   23,   21,   74,   20,
 /*    60 */    19,   18,  117,  134,    5,    6,   86,   87,   66,   67,
 /*    70 */    68,   69,   70,   71,   12,   24,   21,   25,   20,   19,
 /*    80 */    18,   86,   87,   66,   67,   68,   69,   70,   71,   12,
 /*    90 */     7,   21,   26,   20,   19,   18,   27,   96,   66,   67,
 /*   100 */    68,   69,   70,   71,   12,    8,   21,    1,   20,   19,
 /*   110 */    18,  121,   13,   32,    2,   13,   13,   13,   13,   13,
 /*   120 */    13,    3,   31,  121,   13,   86,   87,   13,   13,   13,
 /*   130 */    13,   13,   13,  119,   15,    9,    4,   15,   15,   15,
 /*   140 */    15,   15,   15,  121,   14,   10,   73,   14,   14,   14,
 /*   150 */    14,   14,   14,  101,   11,   28,  101,  101,  101,  101,
 /*   160 */   101,  101,   99,   33,   99,   99,   33,   33,   33,   33,
 /*   170 */    33,   33,   99,   34,   99,   99,   34,   34,   34,   34,
 /*   180 */    34,   34,   30,   29,   28,   86,   87,   78,   99,   99,
 /*   190 */    99,   17,   17,
};
static const YYCODETYPE yy_lookahead[] = {
 /*     0 */     4,    5,    6,    7,    8,    9,   10,   11,   12,   24,
 /*    10 */    14,   15,   16,   17,   18,   19,   20,    4,    5,    6,
 /*    20 */     7,    8,    9,   10,   11,   12,   24,   14,   15,   16,
 /*    30 */    17,   18,    4,    5,    6,    7,    8,    9,   10,   11,
 /*    40 */    12,   24,   14,   15,   16,   17,   18,    4,    5,    6,
 /*    50 */     7,    8,    9,   10,   11,   12,   24,   14,   15,   16,
 /*    60 */    17,   18,    0,   22,   23,   26,    4,    5,    6,    7,
 /*    70 */     8,    9,   10,   11,   12,   24,   14,   24,   16,   17,
 /*    80 */    18,    4,    5,    6,    7,    8,    9,   10,   11,   12,
 /*    90 */    26,   14,   24,   16,   17,   18,   24,    5,    6,    7,
 /*   100 */     8,    9,   10,   11,   12,   26,   14,   26,   16,   17,
 /*   110 */    18,   24,   25,   34,   26,   28,   29,   30,   31,   32,
 /*   120 */    33,   26,   35,   24,   25,    4,    5,   28,   29,   30,
 /*   130 */    31,   32,   33,   24,   25,   27,   26,   28,   29,   30,
 /*   140 */    31,   32,   33,   24,   25,   27,   13,   28,   29,   30,
 /*   150 */    31,   32,   33,   25,   27,    3,   28,   29,   30,   31,
 /*   160 */    32,   33,   36,   25,   36,   36,   28,   29,   30,   31,
 /*   170 */    32,   33,   36,   25,   36,   36,   28,   29,   30,   31,
 /*   180 */    32,   33,    1,    2,    3,    4,    5,   15,   36,   36,
 /*   190 */    36,   19,   20,
};
#define YY_SHIFT_USE_DFLT (193)
#define YY_SHIFT_COUNT    (34)
#define YY_SHIFT_MIN      (-4)
#define YY_SHIFT_MAX      (181)
static const short yy_shift_ofst[] = {
 /*     0 */   193,   -4,   13,   28,   43,   62,   77,   77,   77,   92,
 /*    10 */    92,   92,  193,  181,  181,  181,  121,  121,  121,  121,
 /*    20 */   121,  121,  193,  193,  193,  193,  193,  193,  193,  193,
 /*    30 */   193,  172,  133,  152,  152,
};
#define YY_REDUCE_USE_DFLT (-16)
#define YY_REDUCE_COUNT (30)
#define YY_REDUCE_MIN   (-15)
#define YY_REDUCE_MAX   (148)
static const short yy_reduce_ofst[] = {
 /*     0 */    41,   87,   99,   99,   99,  109,   99,   99,  119,  128,
 /*    10 */   138,  148,   79,  -15,  -15,    2,   17,   32,   51,   53,
 /*    20 */    68,   72,   39,   64,   81,   88,   95,  110,  108,  118,
 /*    30 */   127,
};
static const YYACTIONTYPE yy_default[] = {
 /*     0 */   118,  133,  133,  133,  133,  133,  115,  116,  130,  133,
 /*    10 */   133,  133,  120,  133,  108,  133,  133,  133,  133,  133,
 /*    20 */   133,  133,  120,  120,  120,  120,  120,  120,  131,  131,
 /*    30 */   131,  133,  133,  100,   99,
};
/********** End of lemon-generated parsing tables *****************************/

/* The next table maps tokens (terminal symbols) into fallback tokens.  
** If a construct like the following:
** 
**      %fallback ID X Y Z.
**
** appears in the grammar, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
**
** This feature can be used, for example, to cause some keywords in a language
** to revert to identifiers if they keyword does not apply in the context where
** it appears.
*/
#ifdef YYFALLBACK
const YYCODETYPE yyFallback[] = {
};
#endif /* YYFALLBACK */

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
**
** After the "shift" half of a SHIFTREDUCE action, the stateno field
** actually contains the reduce action for the second half of the
** SHIFTREDUCE.
*/
struct yyStackEntry {
  YYACTIONTYPE stateno;  /* The state-number, or reduce action in SHIFTREDUCE */
  YYCODETYPE major;      /* The major token value.  This is the code
                         ** number for the token at this stack level */
  YYMINORTYPE minor;     /* The user-supplied minor token value.  This
                         ** is the value of the token  */
};

/* The state of the parser is completely contained in an instance of
** the following structure */

#ifndef LEMON_SUPER
#error "LEMON_SUPER must be defined."
#endif

/* outside the class so the templates above are still accessible */
void yy_destructor(YYCODETYPE yymajor, YYMINORTYPE *yypminor);
void yy_move(YYCODETYPE yymajor, YYMINORTYPE *yyDest, YYMINORTYPE *yySource);

class yypParser : public LEMON_SUPER {
  public:
    //using LEMON_SUPER::LEMON_SUPER;

    template<class ...Args>
    yypParser(Args&&... args);

    virtual ~yypParser() override final;
    virtual void parse(int, ParseTOKENTYPE &&) override final;

#ifndef NDEBUG
    virtual void trace(FILE *, const char *) final override;
#endif

    virtual void reset() final override;
    virtual bool will_accept() const final override;

    /*
    ** Return the peak depth of the stack for a parser.
    */
    #ifdef YYTRACKMAXSTACKDEPTH
    int yypParser::stack_peak(){
      return yyhwm;
    }
    #endif

    const yyStackEntry *begin() const { return yystack; }
    const yyStackEntry *end() const { return yytos + 1; }

  protected:
  private:
  yyStackEntry *yytos;          /* Pointer to top element of the stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyhwm = 0;                 /* Maximum value of yyidx */
#endif
#ifndef YYNOERRORRECOVERY
  int yyerrcnt = -1;                 /* Shifts left before out of the error */
#endif
#if YYSTACKDEPTH<=0
  int yystksz = 0;                  /* Current side of the stack */
  yyStackEntry *yystack = nullptr;        /* The parser's stack */
  yyStackEntry yystk0;          /* First stack entry */
  int yyGrowStack();
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
  yyStackEntry *yystackEnd;            /* Last entry in the stack */
#endif



  void yy_accept();
  void yy_parse_failed();
  void yy_syntax_error(int yymajor, ParseTOKENTYPE &yyminor);

  void yy_transfer(yyStackEntry *yySource, yyStackEntry *yyDest);

  void yy_pop_parser_stack();
  unsigned yy_find_shift_action(int stateno, YYCODETYPE iLookAhead) const;
  int yy_find_reduce_action(int stateno, YYCODETYPE iLookAhead) const;

  void yy_shift(int yyNewState, int yyMajor, ParseTOKENTYPE &&yypMinor);
  void yy_reduce(unsigned int yyruleno);
  void yyStackOverflow();

#ifndef NDEBUG
  void yyTraceShift(int yyNewState) const;
#else
# define yyTraceShift(X)
#endif


#ifndef NDEBUG
  FILE *yyTraceFILE = 0;
  const char *yyTracePrompt = 0;
#endif /* NDEBUG */

  int yyidx() const {
    return (int)(yytos - yystack);    
  }

};




#ifndef NDEBUG
/* 
** Turn parser tracing on by giving a stream to which to write the trace
** and a prompt to preface each trace message.  Tracing is turned off
** by making either argument NULL 
**
** Inputs:
** <ul>
** <li> A FILE* to which trace output should be written.
**      If NULL, then tracing is turned off.
** <li> A prefix string written at the beginning of every
**      line of trace output.  If NULL, then tracing is
**      turned off.
** </ul>
**
** Outputs:
** None.
*/
void yypParser::trace(FILE *TraceFILE, const char *zTracePrompt){
  yyTraceFILE = TraceFILE;
  yyTracePrompt = zTracePrompt;
  if( yyTraceFILE==0 ) yyTracePrompt = 0;
  else if( yyTracePrompt==0 ) yyTraceFILE = 0;
}
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing shifts, the names of all terminals and nonterminals
** are required.  The following table supplies these names */
const char *const yyTokenName[] = { 
  "$",             "PIPE_PIPE",     "AMP_AMP",       "PIPE",        
  "SEMI",          "NL",            "COMMAND",       "EVALUATE",    
  "BREAK",         "CONTINUE",      "EXIT",          "ERROR",       
  "LPAREN",        "RPAREN",        "BEGIN",         "END",         
  "LOOP",          "FOR",           "IF",            "ELSE_IF",     
  "ELSE",          "error",         "start",         "command_list",
  "sep",           "command",       "compound_list",  "opt_nl",      
  "term",          "if_command",    "begin_command",  "paren_command",
  "loop_command",  "for_command",   "paren_list",    "else_command",
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
const char *const yyRuleName[] = {
 /*   0 */ "command_list ::= command_list command sep",
 /*   1 */ "compound_list ::= compound_list command sep",
 /*   2 */ "command ::= command PIPE_PIPE opt_nl command",
 /*   3 */ "command ::= command AMP_AMP opt_nl command",
 /*   4 */ "command ::= command PIPE opt_nl command",
 /*   5 */ "term ::= COMMAND",
 /*   6 */ "term ::= EVALUATE",
 /*   7 */ "term ::= BREAK",
 /*   8 */ "term ::= CONTINUE",
 /*   9 */ "term ::= EXIT",
 /*  10 */ "term ::= ERROR",
 /*  11 */ "paren_list ::= compound_list command",
 /*  12 */ "paren_command ::= LPAREN paren_list RPAREN",
 /*  13 */ "begin_command ::= BEGIN sep compound_list END",
 /*  14 */ "loop_command ::= LOOP sep compound_list END",
 /*  15 */ "for_command ::= FOR sep compound_list END",
 /*  16 */ "if_command ::= IF sep compound_list END",
 /*  17 */ "if_command ::= IF sep compound_list else_command END",
 /*  18 */ "else_command ::= ELSE_IF|ELSE sep compound_list",
 /*  19 */ "else_command ::= else_command ELSE_IF|ELSE sep compound_list",
 /*  20 */ "start ::= command_list",
 /*  21 */ "command_list ::=",
 /*  22 */ "command_list ::= command_list sep",
 /*  23 */ "compound_list ::=",
 /*  24 */ "compound_list ::= compound_list sep",
 /*  25 */ "sep ::= SEMI",
 /*  26 */ "sep ::= NL",
 /*  27 */ "command ::= term",
 /*  28 */ "term ::= if_command",
 /*  29 */ "term ::= begin_command",
 /*  30 */ "term ::= paren_command",
 /*  31 */ "term ::= loop_command",
 /*  32 */ "term ::= for_command",
 /*  33 */ "paren_list ::= compound_list",
 /*  34 */ "opt_nl ::=",
 /*  35 */ "opt_nl ::= opt_nl NL",
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.  Return the number
** of errors.  Return 0 on success.
*/
int yypParser::yyGrowStack(){
  int newSize;
  yyStackEntry *pNew;
  yyStackEntry *pOld = yystack;
  int oldSize = yystksz;

  newSize = oldSize*2 + 100;
  pNew = (yyStackEntry *)calloc(newSize, sizeof(pNew[0]));
  if( pNew ){
    yystack = pNew;
    for (int i = 0; i < oldSize; ++i) {
      pNew[i].stateno = pOld[i].stateno;
      pNew[i].major = pOld[i].major;
      yy_move(pOld[i].major, &pNew[i].minor, &pOld[i].minor);
    }
    if (pOld != &yystk0) free(pOld);
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows from %d to %d entries.\n",
              yyTracePrompt, yystksz, newSize);
    }
#endif
    yystksz = newSize;
  }
  return pNew==0; 
}
#endif


/* The following function deletes the "minor type" or semantic value
** associated with a symbol.  The symbol can be either a terminal
** or nonterminal. "yymajor" is the symbol code, and "yypminor" is
** a pointer to the value to be deleted.  The code used to do the 
** deletions is derived from the %destructor and/or %token_destructor
** directives of the input grammar.
*/
void yy_destructor(
  YYCODETYPE yymajor,     /* Type code for object to destroy */
  YYMINORTYPE *yypminor   /* The object to be destroyed */
){
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is 
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are *not* used
    ** inside the C code.
    */
/********* Begin destructor definitions ***************************************/
    case 1: /* PIPE_PIPE */
    case 2: /* AMP_AMP */
    case 3: /* PIPE */
    case 4: /* SEMI */
    case 5: /* NL */
    case 6: /* COMMAND */
    case 7: /* EVALUATE */
    case 8: /* BREAK */
    case 9: /* CONTINUE */
    case 10: /* EXIT */
    case 11: /* ERROR */
    case 12: /* LPAREN */
    case 13: /* RPAREN */
    case 14: /* BEGIN */
    case 15: /* END */
    case 16: /* LOOP */
    case 17: /* FOR */
    case 18: /* IF */
    case 19: /* ELSE_IF */
    case 20: /* ELSE */
      yy_destructor<std::string>(std::addressof(yypminor->yy0));
      break;
    case 0: /* $ */
    case 22: /* start */
    case 23: /* command_list */
      yy_destructor<void>(std::addressof(yypminor->yy7));
      break;
    case 24: /* sep */
    case 25: /* command */
    case 27: /* opt_nl */
    case 28: /* term */
    case 29: /* if_command */
    case 30: /* begin_command */
    case 31: /* paren_command */
    case 32: /* loop_command */
    case 33: /* for_command */
      yy_destructor<command_ptr>(std::addressof(yypminor->yy17));
      break;
    case 26: /* compound_list */
    case 34: /* paren_list */
      yy_destructor< command_ptr_vector >(std::addressof(yypminor->yy35));
      break;
    case 35: /* else_command */
      yy_destructor< if_command::clause_vector_type >(std::addressof(yypminor->yy62));
      break;
/********* End destructor definitions *****************************************/
    default:  break;   /* If no destructor action specified: do nothing */
  }
}


/*
 * moves an object (such as when growing the stack). 
 * Source is constructed.
 * Destination is also destructed.
 * 
 */
void yy_move(
  YYCODETYPE yymajor,     /* Type code for object to move */
  YYMINORTYPE *yyDest,     /*  */
  YYMINORTYPE *yySource     /*  */
){
  switch( yymajor ){

/********* Begin move definitions ***************************************/
    case 1: /* PIPE_PIPE */
    case 2: /* AMP_AMP */
    case 3: /* PIPE */
    case 4: /* SEMI */
    case 5: /* NL */
    case 6: /* COMMAND */
    case 7: /* EVALUATE */
    case 8: /* BREAK */
    case 9: /* CONTINUE */
    case 10: /* EXIT */
    case 11: /* ERROR */
    case 12: /* LPAREN */
    case 13: /* RPAREN */
    case 14: /* BEGIN */
    case 15: /* END */
    case 16: /* LOOP */
    case 17: /* FOR */
    case 18: /* IF */
    case 19: /* ELSE_IF */
    case 20: /* ELSE */
      yy_move<std::string>(std::addressof(yyDest->yy0), std::addressof(yySource->yy0));
      break;
    case 0: /* $ */
    case 22: /* start */
    case 23: /* command_list */
      yy_move<void>(std::addressof(yyDest->yy7), std::addressof(yySource->yy7));
      break;
    case 24: /* sep */
    case 25: /* command */
    case 27: /* opt_nl */
    case 28: /* term */
    case 29: /* if_command */
    case 30: /* begin_command */
    case 31: /* paren_command */
    case 32: /* loop_command */
    case 33: /* for_command */
      yy_move<command_ptr>(std::addressof(yyDest->yy17), std::addressof(yySource->yy17));
      break;
    case 26: /* compound_list */
    case 34: /* paren_list */
      yy_move< command_ptr_vector >(std::addressof(yyDest->yy35), std::addressof(yySource->yy35));
      break;
    case 35: /* else_command */
      yy_move< if_command::clause_vector_type >(std::addressof(yyDest->yy62), std::addressof(yySource->yy62));
      break;
/********* End move definitions *****************************************/
    default:  break;   /* If no move action specified: do nothing */
      //yyDest.minor = yySource.minor;
  }
}


/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
*/
void yypParser::yy_pop_parser_stack(){
  yyStackEntry *yymsp;
  assert( yytos!=0 );
  assert( yytos > yystack );
  yymsp = yytos--;
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yymsp->major]);
  }
#endif
  yy_destructor(yymsp->major, &yymsp->minor);
}


template<class ...Args>
yypParser::yypParser(Args&&... args) : LEMON_SUPER(std::forward<Args>(args)...)
{
#if YYSTACKDEPTH<=0
  if( yyGrowStack() ){
    yystack = &yystk0;
    yystksz = 1;
  }
#else
  std::memset(yystack, 0, sizeof(yystack));
#endif

  yytos = yystack;
  yystack[0].stateno = 0;
  yystack[0].major = 0;
#if YYSTACKDEPTH>0
  yystackEnd = &yystack[YYSTACKDEPTH-1];
#endif
}

void yypParser::reset() {

  while( yytos>yystack ) yy_pop_parser_stack();

#ifndef YYNOERRORRECOVERY
  yyerrcnt = -1;
#endif

  yytos = yystack;
  yystack[0].stateno = 0;
  yystack[0].major = 0;

  LEMON_SUPER::reset();
}


/* 
** Deallocate and destroy a parser.  Destructors are called for
** all stack elements before shutting the parser down.
**
** If the YYPARSEFREENEVERNULL macro exists (for example because it
** is defined in a %include section of the input grammar) then it is
** assumed that the input pointer is never NULL.
*/

yypParser::~yypParser() {
  while( yytos>yystack ) yy_pop_parser_stack();
#if YYSTACKDEPTH<=0
  if( yystack!=&yystk0 ) free(yystack);
#endif
}

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
unsigned yypParser::yy_find_shift_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
) const {
  int i;
 
  if( stateno>=YY_MIN_REDUCE ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
  do{
    i = yy_shift_ofst[stateno];
    assert( iLookAhead!=YYNOCODE );
    i += iLookAhead;
    if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
#ifdef YYFALLBACK
      YYCODETYPE iFallback;            /* Fallback token */
      if( iLookAhead<sizeof(yyFallback)/sizeof(yyFallback[0])
             && (iFallback = yyFallback[iLookAhead])!=0 ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE, "%sFALLBACK %s => %s\n",
             yyTracePrompt, yyTokenName[iLookAhead], yyTokenName[iFallback]);
        }
#endif
        assert( yyFallback[iFallback]==0 ); /* Fallback loop must terminate */
        iLookAhead = iFallback;
        continue;
      }
#endif
#ifdef YYWILDCARD
      {
        int j = i - iLookAhead + YYWILDCARD;
        if( 
#if YY_SHIFT_MIN+YYWILDCARD<0
          j>=0 &&
#endif
#if YY_SHIFT_MAX+YYWILDCARD>=YY_ACTTAB_COUNT
          j<YY_ACTTAB_COUNT &&
#endif
          yy_lookahead[j]==YYWILDCARD && iLookAhead>0
        ){
#ifndef NDEBUG
          if( yyTraceFILE ){
            fprintf(yyTraceFILE, "%sWILDCARD %s => %s\n",
               yyTracePrompt, yyTokenName[iLookAhead],
               yyTokenName[YYWILDCARD]);
          }
#endif /* NDEBUG */
          return yy_action[j];
        }
      }
#endif /* YYWILDCARD */
      return yy_default[stateno];
    }else{
      return yy_action[i];
    }
  }while(1);
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
*/
int yypParser::yy_find_reduce_action(
  int stateno,              /* Current state number */
  YYCODETYPE iLookAhead     /* The look-ahead token */
) const {
  int i;
#ifdef YYERRORSYMBOL
  if( stateno>YY_REDUCE_COUNT ){
    return yy_default[stateno];
  }
#else
  assert( stateno<=YY_REDUCE_COUNT );
#endif
  i = yy_reduce_ofst[stateno];
  assert( i!=YY_REDUCE_USE_DFLT );
  assert( iLookAhead!=YYNOCODE );
  i += iLookAhead;
#ifdef YYERRORSYMBOL
  if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }
#else
  assert( i>=0 && i<YY_ACTTAB_COUNT );
  assert( yy_lookahead[i]==iLookAhead );
#endif
  return yy_action[i];
}

/*
** The following routine is called if the stack overflows.
*/
void yypParser::yyStackOverflow(){
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yytos>yystack ) yy_pop_parser_stack();
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/
/******** End %stack_overflow code ********************************************/
  LEMON_SUPER::stack_overflow();
}

/*
** Print tracing information for a SHIFT action
*/
#ifndef NDEBUG
void yypParser::yyTraceShift(int yyNewState) const {
  if( yyTraceFILE ){
    if( yyNewState<YYNSTATE ){
      fprintf(yyTraceFILE,"%sShift '%s', go to state %d\n",
         yyTracePrompt,yyTokenName[yytos->major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%sShift '%s'\n",
         yyTracePrompt,yyTokenName[yytos->major]);
    }
  }
}
#endif

/*
** Perform a shift action.
*/
void yypParser::yy_shift(
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  ParseTOKENTYPE &&yyMinor      /* The minor token to shift in */
){
  yytos++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yyidx()>yyhwm ){
    yyhwm++;
    assert(yyhwm == yyidx());
  }
#endif
#if YYSTACKDEPTH>0 
  if( yytos>yystackEnd ){
    yytos--;
    yyStackOverflow();
    return;
  }
#else
  if( yytos>=&yystack[yystksz] ){
    if( yyGrowStack() ){
      yytos--;
      yyStackOverflow();
      return;
    }
  }
#endif
  if( yyNewState > YY_MAX_SHIFT ){
    yyNewState += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
  }
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  //yytos->minor.yy0 = yyMinor;
  //yy_move also calls the destructor...
  //yy_move<ParseTOKENTYPE>(std::addressof(yytos->minor.yy0), std::addressof(yyMinor));
  yy_constructor<ParseTOKENTYPE>(std::addressof(yytos->minor.yy0), std::move(yyMinor));
  yyTraceShift(yyNewState);
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static const struct {
  YYCODETYPE lhs;       /* Symbol on the left-hand side of the rule */
  signed char nrhs;     /* Negative of the number of RHS symbols in the rule */
} yyRuleInfo[] = {
  { 23, -3 },
  { 26, -3 },
  { 25, -4 },
  { 25, -4 },
  { 25, -4 },
  { 28, -1 },
  { 28, -1 },
  { 28, -1 },
  { 28, -1 },
  { 28, -1 },
  { 28, -1 },
  { 34, -2 },
  { 31, -3 },
  { 30, -4 },
  { 32, -4 },
  { 33, -4 },
  { 29, -4 },
  { 29, -5 },
  { 35, -3 },
  { 35, -4 },
  { 22, -1 },
  { 23, 0 },
  { 23, -2 },
  { 26, 0 },
  { 26, -2 },
  { 24, -1 },
  { 24, -1 },
  { 25, -1 },
  { 28, -1 },
  { 28, -1 },
  { 28, -1 },
  { 28, -1 },
  { 28, -1 },
  { 34, -1 },
  { 27, 0 },
  { 27, -2 },
};

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
void yypParser::yy_reduce(
  unsigned int yyruleno           /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  yymsp = yytos;
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfo[yyruleno].nrhs;
    fprintf(yyTraceFILE, "%sReduce [%s], go to state %d.\n", yyTracePrompt,
      yyRuleName[yyruleno], yymsp[yysize].stateno);
  }
#endif /* NDEBUG */

  /* Check that the stack is large enough to grow by a single entry
  ** if the RHS of the rule is empty.  This ensures that there is room
  ** enough on the stack to push the LHS value */
  if( yyRuleInfo[yyruleno].nrhs==0 ){
#ifdef YYTRACKMAXSTACKDEPTH
    if( yyidx()>yyhwm ){
      yyhwm++;
      assert(yyhwm == yyidx());
    }
#endif
#if YYSTACKDEPTH>0 
    if( yytos>=yystackEnd ){
      yyStackOverflow();
      return;
    }
#else
    if( yytos>=&yystack[yystksz-1] ){
      if( yyGrowStack() ){
        yyStackOverflow();
        return;
      }
      yymsp = yytos;
    }
#endif
  }

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
/********** Begin reduce actions **********************************************/
      case 0: /* command_list ::= command_list command sep */
{
  yy_destructor<void>(std::addressof(yymsp[-2].minor.yy7));
  yy_destructor<command_ptr>(std::addressof(yymsp[0].minor.yy17));
  auto &C=yy_cast< command_ptr >(std::addressof(yymsp[-1].minor.yy17));
#line 95 "phase3.lemon"
{
	if (C) command_queue.emplace_back(std::move(C));
}
#line 1124 "phase3.cpp"
  yy_destructor(C);
  yy_constructor<void>(std::addressof(yymsp[-2].minor.yy7));
}
        break;
      case 1: /* compound_list ::= compound_list command sep */
{
  yy_destructor<command_ptr>(std::addressof(yymsp[0].minor.yy17));
  auto &L=yy_cast< command_ptr_vector >(std::addressof(yymsp[-2].minor.yy35));
  auto &C=yy_cast< command_ptr >(std::addressof(yymsp[-1].minor.yy17));
#line 111 "phase3.lemon"
{
	if (C) L.emplace_back(std::move(C));
}
#line 1138 "phase3.cpp"
  yy_destructor(C);
}
        break;
      case 2: /* command ::= command PIPE_PIPE opt_nl command */
{
  yy_destructor<std::string>(std::addressof(yymsp[-2].minor.yy0));
  yy_destructor<command_ptr>(std::addressof(yymsp[-1].minor.yy17));
   command_ptr  RV;
  auto &L=yy_cast< command_ptr >(std::addressof(yymsp[-3].minor.yy17));
  auto &R=yy_cast< command_ptr >(std::addressof(yymsp[0].minor.yy17));
#line 124 "phase3.lemon"
{
	RV = std::make_unique<or_command>(std::move(L), std::move(R));
}
#line 1153 "phase3.cpp"
  yy_destructor(L);
  yy_destructor(R);
  yy_constructor< command_ptr >(std::addressof(yymsp[-3].minor.yy17), std::move(RV));
}
        break;
      case 3: /* command ::= command AMP_AMP opt_nl command */
{
  yy_destructor<std::string>(std::addressof(yymsp[-2].minor.yy0));
  yy_destructor<command_ptr>(std::addressof(yymsp[-1].minor.yy17));
   command_ptr  RV;
  auto &L=yy_cast< command_ptr >(std::addressof(yymsp[-3].minor.yy17));
  auto &R=yy_cast< command_ptr >(std::addressof(yymsp[0].minor.yy17));
#line 128 "phase3.lemon"
{
	RV = std::make_unique<and_command>(std::move(L), std::move(R));
}
#line 1170 "phase3.cpp"
  yy_destructor(L);
  yy_destructor(R);
  yy_constructor< command_ptr >(std::addressof(yymsp[-3].minor.yy17), std::move(RV));
}
        break;
      case 4: /* command ::= command PIPE opt_nl command */
{
  yy_destructor<std::string>(std::addressof(yymsp[-2].minor.yy0));
  yy_destructor<command_ptr>(std::addressof(yymsp[-1].minor.yy17));
   command_ptr  RV;
  auto &L=yy_cast< command_ptr >(std::addressof(yymsp[-3].minor.yy17));
  auto &R=yy_cast< command_ptr >(std::addressof(yymsp[0].minor.yy17));
#line 132 "phase3.lemon"
{
	RV = std::make_unique<pipe_command>(std::move(L), std::move(R));
}
#line 1187 "phase3.cpp"
  yy_destructor(L);
  yy_destructor(R);
  yy_constructor< command_ptr >(std::addressof(yymsp[-3].minor.yy17), std::move(RV));
}
        break;
      case 5: /* term ::= COMMAND */
{
  command_ptr RV;
  auto &C=yy_cast<std::string>(std::addressof(yymsp[0].minor.yy0));
#line 138 "phase3.lemon"
{ RV = std::make_unique<simple_command>(std::move(C)); }
#line 1199 "phase3.cpp"
  yy_destructor(C);
  yy_constructor<command_ptr>(std::addressof(yymsp[0].minor.yy17), std::move(RV));
}
        break;
      case 6: /* term ::= EVALUATE */
{
  command_ptr RV;
  auto &C=yy_cast<std::string>(std::addressof(yymsp[0].minor.yy0));
#line 139 "phase3.lemon"
{ RV = std::make_unique<evaluate_command>(std::move(C)); }
#line 1210 "phase3.cpp"
  yy_destructor(C);
  yy_constructor<command_ptr>(std::addressof(yymsp[0].minor.yy17), std::move(RV));
}
        break;
      case 7: /* term ::= BREAK */
{
  command_ptr RV;
  auto &C=yy_cast<std::string>(std::addressof(yymsp[0].minor.yy0));
#line 140 "phase3.lemon"
{ RV = std::make_unique<break_command>(std::move(C)); }
#line 1221 "phase3.cpp"
  yy_destructor(C);
  yy_constructor<command_ptr>(std::addressof(yymsp[0].minor.yy17), std::move(RV));
}
        break;
      case 8: /* term ::= CONTINUE */
{
  command_ptr RV;
  auto &C=yy_cast<std::string>(std::addressof(yymsp[0].minor.yy0));
#line 141 "phase3.lemon"
{ RV = std::make_unique<continue_command>(std::move(C)); }
#line 1232 "phase3.cpp"
  yy_destructor(C);
  yy_constructor<command_ptr>(std::addressof(yymsp[0].minor.yy17), std::move(RV));
}
        break;
      case 9: /* term ::= EXIT */
{
  command_ptr RV;
  auto &C=yy_cast<std::string>(std::addressof(yymsp[0].minor.yy0));
#line 142 "phase3.lemon"
{ RV = std::make_unique<exit_command>(std::move(C)); }
#line 1243 "phase3.cpp"
  yy_destructor(C);
  yy_constructor<command_ptr>(std::addressof(yymsp[0].minor.yy17), std::move(RV));
}
        break;
      case 10: /* term ::= ERROR */
{
  command_ptr RV;
  auto &C=yy_cast<std::string>(std::addressof(yymsp[0].minor.yy0));
  const int yymsp_1_major = yymsp[0].major; /* @C */
#line 151 "phase3.lemon"
{
	RV = std::make_unique<error_command>(yymsp_1_major, std::move(C));
}
#line 1257 "phase3.cpp"
  yy_destructor(C);
  yy_constructor<command_ptr>(std::addressof(yymsp[0].minor.yy17), std::move(RV));
}
        break;
      case 11: /* paren_list ::= compound_list command */
{
  auto &L=yy_cast< command_ptr_vector >(std::addressof(yymsp[-1].minor.yy35));
  auto &C=yy_cast< command_ptr >(std::addressof(yymsp[0].minor.yy17));
#line 180 "phase3.lemon"
{
	L.emplace_back(std::move(C));
}
#line 1270 "phase3.cpp"
  yy_destructor(C);
}
        break;
      case 12: /* paren_command ::= LPAREN paren_list RPAREN */
{
  command_ptr RV;
  auto &T=yy_cast<std::string>(std::addressof(yymsp[-2].minor.yy0));
  auto &L=yy_cast< command_ptr_vector >(std::addressof(yymsp[-1].minor.yy35));
  auto &E=yy_cast<std::string>(std::addressof(yymsp[0].minor.yy0));
  const int yymsp_1_major = yymsp[-2].major; /* @T */
#line 184 "phase3.lemon"
{
	RV = std::make_unique<begin_command>(yymsp_1_major, std::move(L), std::move(T), std::move(E));
}
#line 1285 "phase3.cpp"
  yy_destructor(T);
  yy_destructor(L);
  yy_destructor(E);
  yy_constructor<command_ptr>(std::addressof(yymsp[-2].minor.yy17), std::move(RV));
}
        break;
      case 13: /* begin_command ::= BEGIN sep compound_list END */
{
  yy_destructor<command_ptr>(std::addressof(yymsp[-2].minor.yy17));
  command_ptr RV;
  auto &T=yy_cast<std::string>(std::addressof(yymsp[-3].minor.yy0));
  auto &L=yy_cast< command_ptr_vector >(std::addressof(yymsp[-1].minor.yy35));
  auto &E=yy_cast<std::string>(std::addressof(yymsp[0].minor.yy0));
  const int yymsp_1_major = yymsp[-3].major; /* @T */
#line 189 "phase3.lemon"
{
	RV = std::make_unique<begin_command>(yymsp_1_major, std::move(L), std::move(T), std::move(E));
}
#line 1304 "phase3.cpp"
  yy_destructor(T);
  yy_destructor(L);
  yy_destructor(E);
  yy_constructor<command_ptr>(std::addressof(yymsp[-3].minor.yy17), std::move(RV));
}
        break;
      case 14: /* loop_command ::= LOOP sep compound_list END */
{
  yy_destructor<command_ptr>(std::addressof(yymsp[-2].minor.yy17));
  command_ptr RV;
  auto &T=yy_cast<std::string>(std::addressof(yymsp[-3].minor.yy0));
  auto &L=yy_cast< command_ptr_vector >(std::addressof(yymsp[-1].minor.yy35));
  auto &E=yy_cast<std::string>(std::addressof(yymsp[0].minor.yy0));
  const int yymsp_1_major = yymsp[-3].major; /* @T */
#line 194 "phase3.lemon"
{
	RV = std::make_unique<loop_command>(yymsp_1_major, std::move(L), std::move(T), std::move(E));
}
#line 1323 "phase3.cpp"
  yy_destructor(T);
  yy_destructor(L);
  yy_destructor(E);
  yy_constructor<command_ptr>(std::addressof(yymsp[-3].minor.yy17), std::move(RV));
}
        break;
      case 15: /* for_command ::= FOR sep compound_list END */
{
  yy_destructor<command_ptr>(std::addressof(yymsp[-2].minor.yy17));
  command_ptr RV;
  auto &T=yy_cast<std::string>(std::addressof(yymsp[-3].minor.yy0));
  auto &L=yy_cast< command_ptr_vector >(std::addressof(yymsp[-1].minor.yy35));
  auto &E=yy_cast<std::string>(std::addressof(yymsp[0].minor.yy0));
  const int yymsp_1_major = yymsp[-3].major; /* @T */
#line 198 "phase3.lemon"
{
	RV = std::make_unique<for_command>(yymsp_1_major, std::move(L), std::move(T), std::move(E));
}
#line 1342 "phase3.cpp"
  yy_destructor(T);
  yy_destructor(L);
  yy_destructor(E);
  yy_constructor<command_ptr>(std::addressof(yymsp[-3].minor.yy17), std::move(RV));
}
        break;
      case 16: /* if_command ::= IF sep compound_list END */
{
  yy_destructor<command_ptr>(std::addressof(yymsp[-2].minor.yy17));
  command_ptr RV;
  auto &I=yy_cast<std::string>(std::addressof(yymsp[-3].minor.yy0));
  auto &L=yy_cast< command_ptr_vector >(std::addressof(yymsp[-1].minor.yy35));
  auto &E=yy_cast<std::string>(std::addressof(yymsp[0].minor.yy0));
#line 202 "phase3.lemon"
{

	if_command::clause_vector_type v;
	v.emplace_back(std::make_unique<if_else_clause>(IF, std::move(L), std::move(I)));

	RV = std::make_unique<if_command>(
		std::move(v),
		std::move(E)
	);

}
#line 1368 "phase3.cpp"
  yy_destructor(I);
  yy_destructor(L);
  yy_destructor(E);
  yy_constructor<command_ptr>(std::addressof(yymsp[-3].minor.yy17), std::move(RV));
}
        break;
      case 17: /* if_command ::= IF sep compound_list else_command END */
{
  yy_destructor<command_ptr>(std::addressof(yymsp[-3].minor.yy17));
  command_ptr RV;
  auto &I=yy_cast<std::string>(std::addressof(yymsp[-4].minor.yy0));
  auto &L=yy_cast< command_ptr_vector >(std::addressof(yymsp[-2].minor.yy35));
  auto &EC=yy_cast< if_command::clause_vector_type >(std::addressof(yymsp[-1].minor.yy62));
  auto &E=yy_cast<std::string>(std::addressof(yymsp[0].minor.yy0));
#line 214 "phase3.lemon"
{

	if_command::clause_vector_type v;
	v.emplace_back(std::make_unique<if_else_clause>(IF, std::move(L), std::move(I)));
	for(auto &c : EC) { v.emplace_back(std::move(c)); }

	RV = std::make_unique<if_command>(
		std::move(v), std::move(E));	
}
#line 1393 "phase3.cpp"
  yy_destructor(I);
  yy_destructor(L);
  yy_destructor(EC);
  yy_destructor(E);
  yy_constructor<command_ptr>(std::addressof(yymsp[-4].minor.yy17), std::move(RV));
}
        break;
      case 18: /* else_command ::= ELSE_IF|ELSE sep compound_list */
{
  yy_destructor<command_ptr>(std::addressof(yymsp[-1].minor.yy17));
   if_command::clause_vector_type  RV;
  auto &E=yy_cast<std::string>(std::addressof(yymsp[-2].minor.yy0));
  auto &L=yy_cast< command_ptr_vector >(std::addressof(yymsp[0].minor.yy35));
  const int yymsp_1_major = yymsp[-2].major; /* @E */
#line 227 "phase3.lemon"
{
	RV.emplace_back(std::make_unique<if_else_clause>(yymsp_1_major, std::move(L), std::move(E)));
}
#line 1412 "phase3.cpp"
  yy_destructor(E);
  yy_destructor(L);
  yy_constructor< if_command::clause_vector_type >(std::addressof(yymsp[-2].minor.yy62), std::move(RV));
}
        break;
      case 19: /* else_command ::= else_command ELSE_IF|ELSE sep compound_list */
{
  yy_destructor<command_ptr>(std::addressof(yymsp[-1].minor.yy17));
  auto &EC=yy_cast< if_command::clause_vector_type >(std::addressof(yymsp[-3].minor.yy62));
  auto &E=yy_cast<std::string>(std::addressof(yymsp[-2].minor.yy0));
  auto &L=yy_cast< command_ptr_vector >(std::addressof(yymsp[0].minor.yy35));
  const int yymsp_2_major = yymsp[-2].major; /* @E */
#line 232 "phase3.lemon"
{
	EC.emplace_back(std::make_unique<if_else_clause>(yymsp_2_major, std::move(L), std::move(E)));
}
#line 1429 "phase3.cpp"
  yy_destructor(E);
  yy_destructor(L);
}
        break;
      case 20: /* start ::= command_list */
{
  yy_destructor<void>(std::addressof(yymsp[0].minor.yy7));
  yy_constructor<void>(std::addressof(yymsp[0].minor.yy7));
}
        break;
      case 21: /* command_list ::= */
  yy_constructor<void>(std::addressof(yymsp[1].minor.yy7));
        break;
      case 22: /* command_list ::= command_list sep */
{
  yy_destructor<void>(std::addressof(yymsp[-1].minor.yy7));
  yy_destructor<command_ptr>(std::addressof(yymsp[0].minor.yy17));
  yy_constructor<void>(std::addressof(yymsp[-1].minor.yy7));
}
        break;
      case 23: /* compound_list ::= */
  yy_constructor< command_ptr_vector >(std::addressof(yymsp[1].minor.yy35));
        break;
      case 24: /* compound_list ::= compound_list sep */
{
  yy_destructor<command_ptr>(std::addressof(yymsp[0].minor.yy17));
}
        break;
      case 25: /* sep ::= SEMI */
      case 26: /* sep ::= NL */ yytestcase(yyruleno==26);
{
  yy_destructor<std::string>(std::addressof(yymsp[0].minor.yy0));
  yy_constructor<command_ptr>(std::addressof(yymsp[0].minor.yy17));
}
        break;
      case 34: /* opt_nl ::= */
  yy_constructor<command_ptr>(std::addressof(yymsp[1].minor.yy17));
        break;
      case 35: /* opt_nl ::= opt_nl NL */
{
  yy_destructor<command_ptr>(std::addressof(yymsp[-1].minor.yy17));
  yy_destructor<std::string>(std::addressof(yymsp[0].minor.yy0));
  yy_constructor<command_ptr>(std::addressof(yymsp[-1].minor.yy17));
}
        break;
      default:
      /* (27) command ::= term (OPTIMIZED OUT) */ assert(yyruleno!=27);
      /* (28) term ::= if_command (OPTIMIZED OUT) */ assert(yyruleno!=28);
      /* (29) term ::= begin_command (OPTIMIZED OUT) */ assert(yyruleno!=29);
      /* (30) term ::= paren_command (OPTIMIZED OUT) */ assert(yyruleno!=30);
      /* (31) term ::= loop_command (OPTIMIZED OUT) */ assert(yyruleno!=31);
      /* (32) term ::= for_command (OPTIMIZED OUT) */ assert(yyruleno!=32);
      /* (33) paren_list ::= compound_list */ yytestcase(yyruleno==33);
        break;
/********** End reduce actions ************************************************/
  };
  assert( yyruleno<sizeof(yyRuleInfo)/sizeof(yyRuleInfo[0]) );
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yyact = yy_find_reduce_action(yymsp[yysize].stateno,(YYCODETYPE)yygoto);

  /* There are no SHIFTREDUCE actions on nonterminals because the table
  ** generator has simplified them to pure REDUCE actions. */
  assert( !(yyact>YY_MAX_SHIFT && yyact<=YY_MAX_SHIFTREDUCE) );

  /* It is not possible for a REDUCE to be followed by an error */
  assert( yyact!=YY_ERROR_ACTION );

  if( yyact==YY_ACCEPT_ACTION ){
    yytos += yysize;
    yy_accept();
  }else{
    yymsp += yysize+1;
    yytos = yymsp;
    yymsp->stateno = (YYACTIONTYPE)yyact;
    yymsp->major = (YYCODETYPE)yygoto;
    yyTraceShift(yyact);
  }
}

/*
** The following code executes when the parse fails
*/
#ifndef YYNOERRORRECOVERY
void yypParser::yy_parse_failed(){
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sFail!\n",yyTracePrompt);
  }
#endif
  while( yytos>yystack ) yy_pop_parser_stack();
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
/************ End %parse_failure code *****************************************/
  LEMON_SUPER::parse_failure();
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
void yypParser::yy_syntax_error(
  int yymajor,                   /* The major type of the error token */
  ParseTOKENTYPE &yyminor        /* The minor type of the error token */
){
//#define TOKEN yyminor
  auto &TOKEN = yyminor;
/************ Begin %syntax_error code ****************************************/
/************ End %syntax_error code ******************************************/
  LEMON_SUPER::syntax_error(yymajor, TOKEN);
}

/*
** The following is executed when the parser accepts
*/
void yypParser::yy_accept(){
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sAccept!\n",yyTracePrompt);
  }
#endif
#ifndef YYNOERRORRECOVERY
  yyerrcnt = -1;
#endif
  assert( yytos==yystack );
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
/*********** End %parse_accept code *******************************************/
  LEMON_SUPER::parse_accept();
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "ParseAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/



void yypParser::parse(
  int yymajor,                 /* The major token code number */
  ParseTOKENTYPE &&yyminor       /* The value for the token */
){
  //YYMINORTYPE yyminorunion;
  unsigned int yyact;            /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif

  assert( yytos!=0 );

#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput '%s'\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action(yytos->stateno, (YYCODETYPE)yymajor);
    if( yyact <= YY_MAX_SHIFTREDUCE ){
      yy_shift(yyact,yymajor,std::move(yyminor));
#ifndef YYNOERRORRECOVERY
      yyerrcnt--;
#endif
      yymajor = YYNOCODE;
    }else if( yyact <= YY_MAX_REDUCE ){
      yy_reduce(yyact-YY_MIN_REDUCE);
    }else{
      assert( yyact == YY_ERROR_ACTION );
#ifdef YYERRORSYMBOL
      int yymx;
#endif
#ifndef NDEBUG
      if( yyTraceFILE ){
        fprintf(yyTraceFILE,"%sSyntax Error!\n",yyTracePrompt);
      }
#endif
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".  
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yyerrcnt<0 ){
        yy_syntax_error(yymajor,yyminor);
      }
      yymx = yytos->major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        //yy_destructor(yyminor);
        yymajor = YYNOCODE;
      }else{
        while( yytos >= yystack
            && yymx != YYERRORSYMBOL
            && (yyact = yy_find_reduce_action(
                        yytos->stateno,
                        YYERRORSYMBOL)) >= YY_MIN_REDUCE
        ){
          yy_pop_parser_stack();
        }
        if( yytos < yystack || yymajor==0 ){
          //yy_destructor(yyminor);
          yy_parse_failed();
#ifndef YYNOERRORRECOVERY
          yyerrcnt = -1;
#endif
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          yy_shift(yyact,YYERRORSYMBOL,std::move(yyminor));
        }
      }
      yyerrcnt = 3;
      yyerrorhit = 1;
#elif defined(YYNOERRORRECOVERY)
      /* If the YYNOERRORRECOVERY macro is defined, then do not attempt to
      ** do any kind of error recovery.  Instead, simply invoke the syntax
      ** error routine and continue going as if nothing had happened.
      **
      ** Applications can set this macro (for example inside %include) if
      ** they intend to abandon the parse upon the first syntax error seen.
      */
      yy_syntax_error(yymajor,yyminor);
      //yy_destructor(yyminor);
      yymajor = YYNOCODE;
      
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yyerrcnt<=0 ){
        yy_syntax_error(yymajor,yyminor);
      }
      yyerrcnt = 3;
      //yy_destructor(yyminor);
      if( yyendofinput ){
        yy_parse_failed();
#ifndef YYNOERRORRECOVERY
        yyerrcnt = -1;
#endif
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yytos>yystack );
#ifndef NDEBUG
  if( yyTraceFILE ){
    yyStackEntry *i;
    char cDiv = '[';
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=&yystack[1]; i<=yytos; i++){
      fprintf(yyTraceFILE,"%c%s", cDiv, yyTokenName[i->major]);
      cDiv = ' ';
    }
    if (cDiv == '[') fprintf(yyTraceFILE,"[");
    fprintf(yyTraceFILE,"]\n");
  }
#endif
  return;
}


bool yypParser::will_accept() const {


  struct stack_entry {
    int stateno;
    int major;
  };

  int yyact;
  const int yymajor = 0;
  std::vector<stack_entry> stack;


  // copy stack to stack.
  stack.reserve(yyidx()+1);
  std::transform(begin(), end(), std::back_inserter(stack), [](const yyStackEntry &e){
    return stack_entry({e.stateno, e.major});
  });

  do {
    yyact = yy_find_shift_action(stack.back().stateno, yymajor);
    if (yyact <= YY_MAX_SHIFTREDUCE) {
      // shift
      return false;
      //stack.push_back({yyact, yymajor});
      //yymajor = YYNOCODE;
    }
    else if (yyact <= YY_MAX_REDUCE) {
      // reduce...
      unsigned yyruleno = yyact - YY_MIN_REDUCE;

      int yygoto = yyRuleInfo[yyruleno].lhs;
      int yysize = -yyRuleInfo[yyruleno].nrhs; /* stored as negative value */

      while (yysize--) stack.pop_back();

      yyact = yy_find_reduce_action(stack.back().stateno,(YYCODETYPE)yygoto);


      if (yyact == YY_ACCEPT_ACTION) return true;

      if( yyact>YY_MAX_SHIFT ){
        yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
      }

      stack.push_back({yyact, yygoto});
    }
    else {
      return false;
    }

  } while (!stack.empty());

  return false;


}




} // namespace
#line 15 "phase3.lemon"

	
std::unique_ptr<phase3> phase3::make() {
	return std::make_unique<yypParser>();
}

bool phase3::continuation() const {
	yypParser *self = (yypParser *)this;

	for (const auto &e : *self) {
		if (e.major == BEGIN) return true;
		if (e.major == LPAREN) return true;
		if (e.major == IF) return true;
		if (e.major == AMP_AMP) return true;
		if (e.major == PIPE_PIPE) return true;
		if (e.major == LOOP) return true;
		if (e.major == FOR) return true;
		if (e.major == PIPE) return true;
		if (e.major == PIPE_PIPE) return true;
		if (e.major == AMP_AMP) return true;
	}
	return false;
}


void phase3::parse_accept() {
	error = false;
}

void phase3::parse_failure() {
	error = false;
}

void phase3::syntax_error(int yymajor, std::string &yyminor) {
/*
	switch (yymajor) {
	case END:
		fprintf(stderr, "### MPW Shell - Extra END command.\n");
		break;

	case RPAREN:
		fprintf(stderr, "### MPW Shell - Extra ) command.\n");
		break;

	case ELSE:
	case ELSE_IF:
		fprintf(stderr, "### MPW Shell - ELSE must be within IF ... END.\n");
		break;

	default:
		fprintf(stderr, "### Parse error near %s\n", yyminor.c_str());
		break;
	}
*/

	
	fprintf(stderr, "### MPW Shell - Parse error near %s\n", yymajor ? yyminor.c_str() : "EOF");
	error = true;
}


#line 1863 "phase3.cpp"
