#ifndef LIZP_H_
#define LIZP_H_
#include <sys/types.h>
/* Lizp is a functional language based on lambda calculus.
 *  
 * - Currying
 * - High level functions
 * - Call by value
 * - Weak type deference
 */

/* Lizp Language Grammar
 * - Lambda Original Syntax
 * <expr>   ::= <lambda>|<application>|<var>
 * <lambda> ::= (\<var>.<expr>)
 * <application>
 *          ::= (<expr> <expr>)
 * <var>    ::= /[^0-9][0-9a-z]+/
 * 
 * - Lizp Syntax 
 * <expr> ::= <var>
 *           |(\<var>.<expr>) -- Lambda
 *           |(<expr> <expr>)  -- Application
 * <def>  ::= def <var> = <expr>
 * <var>  ::= <num>|<str>|<idf>
 * <num>  ::= /-?[0-9]+/
 * <str>  ::= /"[^\"\0]*"/
 * <idf>  ::= /[^0-9][a-zA-Z0-9_-'~!@#$%^&*]+/
 */

/* Lizp Code Sample
 * 
 * - Lambda Defination
 *   (\x.x)
 *   
 * - Lambda Application
 *   ((\x.x) y) 
 *     -> y
 *   
 * - Currying
 *   (\x.(\y.x+y)) z 
 *     -> (\y.z+y) 
 *     
 * - Function Defination
 *   def add1 = (\x.x+1)
 *   
 * - Function Application
 *   (add1 2) 
 *     -> (\x.x+1) 2 
 *       -> 2+1 
 *         -> 3
 *         
 * - High level function defination
 *   - To apply  a lambda
 *     def apply = (\f.(\x.f x))
 *     
 *   - To reduce a lambda
 *     def addn = (\x.(\y.x+y))
 *     
 * - High level function application
 *   - Apply  a lambda
 *     apply add1 2 
 *       -> (\f.(\x.f x)) add1 2 
 *         -> (\x.add1 x) 2 
 *           -> (\x.(\y.y+1) x) 2 
 *             -> (\y.y+1) 2 
 *               -> 2+1 
 *                 -> 3
 *                 
 *   - Reduce a lambda
 *     addn 2 
 *       -> (\y.(\x.x+y)) 2 
 *         -> (\x.x+2)
 *         
 * - Variable Defination
 *   def var = 1
 *   
 *   
 * Future Features
 * 
 * - Function Combinator 
 *   def add = apply addn 
 *   
 * - Recurse Trick
 *   def fact = (\x.x!=0 or 1?x*(self x-1):1) 
 *   ; <?:> is a builtin
 *   ; <self> is a keyword
 */

/* > Actually, We can define everything as lambda(s). 
 * Builtins:
 *  <number>      nature numbers (Church-Number)
 *  <string>      native strings
 *  [+,-,*,/]     algebra operators 
 *  [&,|,~]       bitwise operators
 *  [&&,||,!]     logic operators
 *  [>,<,==,!=]   compare operators
 *  [?:]          symantic for goto
 *  [self]        symantic for recursion 
 *  [cons]        symantic for list constuctor
 */

/* Forward Declarations */

struct mpc_ast_t;
struct val_t;
struct env_t;
typedef struct mpc_ast_t mpc_ast_t;
typedef struct val_t val_t;
typedef struct env_t env_t;

/* Lizp val_tue type */

enum { TYPE_STR, TYPE_ERR, TYPE_NUM, TYPE_SYM, TYPE_LMD, TYPE_APL, TYPE_CLS };

struct val_t {
	/* Basic infomation */
	int type;
	/* Basic values */
	long num;
    /* String/Error/Lambda parameter/Symbol */
	char * s;
	/* For lambda and function */
	val_t * body;
	env_t * innerEnv;

	/* For application */
	val_t * left;
	val_t * right;
};

val_t * valNew(int type);
void    valDel(val_t * val);
val_t * valRead(mpc_ast_t * ast);
val_t * eval(val_t * val, env_t * env);
val_t * apply(val_t * l, val_t * r, env_t * env);

//val_t * eval(val_t * val);


/* An env variable is a symbol table with a 
 * pointer to its father.
 */
struct env_t {
	env_t * parent;
#ifdef FEATURE_JOIN_SYMBOLS
	size_t childrenEnvCount;
	env_t ** children;
#endif
	size_t count;
	char ** syms;
	val_t ** vars;
};

env_t * envNew(void);
void    envDel(env_t * env);
val_t * envFind(const char * sym, env_t * env);
int     envBind(const char * sym, val_t * val, env_t * env);
int     envInsert(const char * sym, val_t * val, env_t * env);
void    envRemove(const char * sym, env_t * env);
void    envPrint(env_t * env);

#endif
