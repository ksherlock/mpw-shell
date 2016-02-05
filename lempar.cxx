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
#include <cassert>
#include <type_traits>
#include <new>
#include <memory>

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
%%
/**************** End of %include directives **********************************/
/* These constants specify the various numeric values for terminal symbols
** in a format understandable to "makeheaders".  This section is blank unless
** "lemon" is run with the "-m" command-line option.
***************** Begin makeheaders token definitions *************************/
%%
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
%%
/************* End control #defines *******************************************/
namespace {
/* The yyzerominor constant is used to initialize instances of
** YYMINORTYPE objects to zero. */
const YYMINORTYPE yyzerominor = { 0 };

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

**   N == YY_ERROR_ACTION               A syntax error has occurred.
**
**   N == YY_ACCEPT_ACTION              The parser accepts its input.
**
**   N == YY_NO_ACTION                  No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.  
**
** The formula above is for computing the action when the lookahead is
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
%%
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
%%
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
    using LEMON_SUPER::LEMON_SUPER;

    virtual ~yypParser() override final;
    virtual void parse(int, ParseTOKENTYPE &&) override final;

    virtual void trace(FILE *, const char *) final override;


    /*
    ** Return the peak depth of the stack for a parser.
    */
    #ifdef YYTRACKMAXSTACKDEPTH
    int yypParser::stack_peak(){
      return yyidxMax;
    }
    #endif

    const yyStackEntry *begin() const { return &yystack[0]; }
    const yyStackEntry *end() const { return &yystack[yyidx > 0 ? yyidx + 1: 0]; }

  protected:
  private:
  bool init = false;
  int yyidx = -1;                    /* Index of top element in stack */
#ifdef YYTRACKMAXSTACKDEPTH
  int yyidxMax = 0;                 /* Maximum value of yyidx */
#endif
  int yyerrcnt = 0;                 /* Shifts left before out of the error */
#if YYSTACKDEPTH<=0
  int yystksz = 0;                  /* Current side of the stack */
  yyStackEntry *yystack = nullptr;        /* The parser's stack */
  void yyGrowStack();
#else
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
#endif



  void initialize();
  void yy_accept();
  void yy_parse_failed();
  void yy_syntax_error(int yymajor, YYMINORTYPE &yyminor);

  void yy_transfer(yyStackEntry *yySource, yyStackEntry *yyDest);

  void yy_pop_parser_stack();
  int yy_find_shift_action(YYCODETYPE iLookAhead) const;

  void yy_shift(int yyNewState, int yyMajor, YYMINORTYPE *yypMinor);
  void yy_reduce(int yyruleno);
  static int yy_find_reduce_action(int stateno, YYCODETYPE iLookAhead);
  void yyStackOverflow(YYMINORTYPE *yypMinor);

#ifndef NDEBUG
  void yyTraceShift(int yyNewState) const;
#else
# define yyTraceShift(X)
#endif


#ifndef NDEBUG
FILE *yyTraceFILE = 0;
const char *yyTracePrompt = 0;
#endif /* NDEBUG */


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
%%
};
#endif /* NDEBUG */

#ifndef NDEBUG
/* For tracing reduce actions, the names of all rules are required.
*/
const char *const yyRuleName[] = {
%%
};
#endif /* NDEBUG */


#if YYSTACKDEPTH<=0
/*
** Try to increase the size of the parser stack.
*/


void yypParser::yyGrowStack(){
  int newSize;
  yyStackEntry *pNew;
  yyStackEntry *pOld = yystack;
  int oldSize = yystksz;

  newSize = oldSize*2 + 100;
  //pNew = realloc(yystack, newSize*sizeof(pNew[0]));
  pNew = (yyStackEntry *)calloc(newSize, sizeof(pNew[0]));
  if( pNew ){
    yystack = pNew;
    yystksz = newSize;
    for (int i = 0; i < oldSize; ++i) {
      pNew[i].stateno = pOld[i].stateno;
      pNew[i].major = pOld[i].major;
      yy_move(pOld[i].major, &pNew[i].minor, &pOld[i].minor);
    }
    free(pOld);
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sStack grows to %d entries!\n",
              yyTracePrompt, yystksz);
    }
#endif
  }
}
#endif


/*
 * this should be in the constructor, but we inherit the parent's
 * constructors.
 */
void yypParser::initialize() {
  if (!init) {
#if YYSTACKDEPTH<=0
    yyGrowStack();
#else
    memset(yystack, 0, sizeof(yystack));
#endif
  }
  init = true;
}

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
%%
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
%%
/********* End move &&definitions *****************************************/
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
  yyStackEntry *yytos;
  assert( yyidx>=0 );
  yytos = &yystack[yyidx--];
#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sPopping %s\n",
      yyTracePrompt,
      yyTokenName[yytos->major]);
  }
#endif
  yy_destructor(yytos->major, &yytos->minor);
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
  while(yyidx>=0 ) yy_pop_parser_stack();
#if YYSTACKDEPTH<=0
  free(yystack);
#endif
}

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
*/
int yypParser::yy_find_shift_action(
  YYCODETYPE iLookAhead     /* The look-ahead token */
) const {
  int i;
  int stateno = yystack[yyidx].stateno;
 
  if( stateno>=YY_MIN_REDUCE ) return stateno;
  assert( stateno <= YY_SHIFT_COUNT );
  do{
    i = yy_shift_ofst[stateno];
    if( i==YY_SHIFT_USE_DFLT ) return yy_default[stateno];
    assert( iLookAhead!=YYNOCODE );
    i += iLookAhead;
    if( i<0 || i>=YY_ACTTAB_COUNT || yy_lookahead[i]!=iLookAhead ){
      if( iLookAhead>0 ){
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
            yy_lookahead[j]==YYWILDCARD
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
      }
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
){
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
void yypParser::yyStackOverflow(YYMINORTYPE *yypMinor){
   yyidx--;
#ifndef NDEBUG
   if( yyTraceFILE ){
     fprintf(yyTraceFILE,"%sStack Overflow!\n",yyTracePrompt);
   }
#endif
   while( yyidx>=0 ) yy_pop_parser_stack();
   /* Here code is inserted which will execute if the parser
   ** stack every overflows */
/******** Begin %stack_overflow code ******************************************/
%%
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
         yyTracePrompt,yyTokenName[yystack[yyidx].major],
         yyNewState);
    }else{
      fprintf(yyTraceFILE,"%sShift '%s'\n",
         yyTracePrompt,yyTokenName[yystack[yyidx].major]);
    }
  }
}
#else
# define yyTraceShift(X,Y)
#endif

/*
** Perform a shift action.
*/
void yypParser::yy_shift(
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer to the minor token to shift in */
){
  yyStackEntry *yytos;
  yyidx++;
#ifdef YYTRACKMAXSTACKDEPTH
  if( yyidx>yyidxMax ){
    yyidxMax = yyidx;
  }
#endif
#if YYSTACKDEPTH>0 
  if( yyidx>=YYSTACKDEPTH ){
    yyStackOverflow(yypMinor);
    return;
  }
#else
  if( yyidx>=yystksz ){
    yyGrowStack();
    if( yyidx>=yystksz ){
      yyStackOverflow(yypMinor);
      return;
    }
  }
#endif
  yytos = &yystack[yyidx];
  yytos->stateno = (YYACTIONTYPE)yyNewState;
  yytos->major = (YYCODETYPE)yyMajor;
  //yytos->minor = *yypMinor;
  yy_move(yyMajor, &yytos->minor, yypMinor);
  yyTraceShift(yyNewState);
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
const struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
%%
};

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
void yypParser::yy_reduce(
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  yymsp = &yystack[yyidx];
#ifndef NDEBUG
  if( yyTraceFILE && yyruleno>=0 
        && yyruleno<(int)(sizeof(yyRuleName)/sizeof(yyRuleName[0])) ){
    yysize = yyRuleInfo[yyruleno].nrhs;
    fprintf(yyTraceFILE, "%sReduce [%s], go to state %d.\n", yyTracePrompt,
      yyRuleName[yyruleno], yymsp[-yysize].stateno);
  }
#endif /* NDEBUG */
  yygotominor = yyzerominor;

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
%%
/********** End reduce actions ************************************************/
  };
  assert( yyruleno>=0 && yyruleno<sizeof(yyRuleInfo)/sizeof(yyRuleInfo[0]) );
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yyidx -= yysize;
  yyact = yy_find_reduce_action(yymsp[-yysize].stateno,(YYCODETYPE)yygoto);
  if( yyact <= YY_MAX_SHIFTREDUCE ){
    if( yyact>YY_MAX_SHIFT ) yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
    /* If the reduce action popped at least
    ** one element off the stack, then we can push the new element back
    ** onto the stack here, and skip the stack overflow test in yy_shift().
    ** That gives a significant speed improvement. */
    if( yysize ){
      yyidx++;
      yymsp -= yysize-1;
      yymsp->stateno = (YYACTIONTYPE)yyact;
      yymsp->major = (YYCODETYPE)yygoto;
      //yymsp->minor = yygotominor;
      yy_move(yygoto, &yymsp->minor, &yygotominor);
      yyTraceShift(yyact);
    }else{
      yy_shift(yyact,yygoto,&yygotominor);
    }
  }else{
    assert( yyact == YY_ACCEPT_ACTION );
    yy_accept();
    //yy_destructor(yygoto, &yygotominor); // ??? only an issue if terminal has rhs?
    // yygotominor will not be deletable if now rhs.
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
  while( yyidx>=0 ) yy_pop_parser_stack();
  /* Here code is inserted which will be executed whenever the
  ** parser fails */
/************ Begin %parse_failure code ***************************************/
%%
/************ End %parse_failure code *****************************************/
  LEMON_SUPER::parse_failure();
}
#endif /* YYNOERRORRECOVERY */

/*
** The following code executes when a syntax error first occurs.
*/
void yypParser::yy_syntax_error(
  int yymajor,                   /* The major type of the error token */
  YYMINORTYPE &yyminor           /* The minor type of the error token */
){
//#define TOKEN (yyminor.yy0)
  auto &TOKEN = yy_cast<ParseTOKENTYPE>(std::addressof(yyminor.yy0));
/************ Begin %syntax_error code ****************************************/
%%
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
  while( yyidx>=0 ) yy_pop_parser_stack();
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
/*********** Begin %parse_accept code *****************************************/
%%
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
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  int yyendofinput;     /* True if we are at the end of input */
#endif
#ifdef YYERRORSYMBOL
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
#endif

  if (!init) initialize();

  /* (re)initialize the parser, if necessary */
  if( yyidx<0 ){
#if YYSTACKDEPTH<=0
    if( yystksz <=0 ){
      /*memset(&yyminorunion, 0, sizeof(yyminorunion));*/
      yyminorunion = yyzerominor;
      yyStackOverflow(&yyminorunion);
      return;
    }
#endif
    yyidx = 0;
    yyerrcnt = -1;
    yystack[0].stateno = 0;
    yystack[0].major = 0;
#ifndef NDEBUG
    if( yyTraceFILE ){
      fprintf(yyTraceFILE,"%sInitialize. Empty stack. State 0\n",
              yyTracePrompt);
    }
#endif
  }
  //yyminorunion.yy0 = yyminor;
  yy_constructor<ParseTOKENTYPE>(std::addressof(yyminorunion.yy0), std::move(yyminor));
#if !defined(YYERRORSYMBOL) && !defined(YYNOERRORRECOVERY)
  yyendofinput = (yymajor==0);
#endif

#ifndef NDEBUG
  if( yyTraceFILE ){
    fprintf(yyTraceFILE,"%sInput '%s'\n",yyTracePrompt,yyTokenName[yymajor]);
  }
#endif

  do{
    yyact = yy_find_shift_action((YYCODETYPE)yymajor);
    if( yyact <= YY_MAX_SHIFTREDUCE ){
      if( yyact > YY_MAX_SHIFT ) yyact += YY_MIN_REDUCE - YY_MIN_SHIFTREDUCE;
      yy_shift(yyact,yymajor,&yyminorunion);
      yyerrcnt--;
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
        yy_syntax_error(yymajor,yyminorunion);
      }
      yymx = yystack[yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
#ifndef NDEBUG
        if( yyTraceFILE ){
          fprintf(yyTraceFILE,"%sDiscard input token %s\n",
             yyTracePrompt,yyTokenName[yymajor]);
        }
#endif
        yy_destructor((YYCODETYPE)yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_reduce_action(
                        yystack[yyidx].stateno,
                        YYERRORSYMBOL)) >= YY_MIN_REDUCE
        ){
          yy_pop_parser_stack();
        }
        if( yyidx < 0 || yymajor==0 ){
          yy_destructor((YYCODETYPE)yymajor,&yyminorunion);
          yy_parse_failed();
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yyact,YYERRORSYMBOL,&u2);
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
      yy_syntax_error(yymajor,yyminorunion);
      yy_destructor((YYCODETYPE)yymajor,&yyminorunion);
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
        yy_syntax_error(yymajor,yyminorunion);
      }
      yyerrcnt = 3;
      yy_destructor((YYCODETYPE)yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed();
      }
      yymajor = YYNOCODE;
#endif
    }
  }while( yymajor!=YYNOCODE && yyidx>=0 );
#ifndef NDEBUG
  if( yyTraceFILE ){
    int i;
    fprintf(yyTraceFILE,"%sReturn. Stack=",yyTracePrompt);
    for(i=1; i<=yyidx; i++)
      fprintf(yyTraceFILE,"%c%s", i==1 ? '[' : ' ', 
              yyTokenName[yystack[i].major]);
    fprintf(yyTraceFILE,"]\n");
  }
#endif
  return;
}

} // namespace