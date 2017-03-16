#include "lizp.h"
#include "mpc.h"
#include <editline/readline.h>
#include <stdio.h>
#include <stdlib.h>

static env_t * GlobalEnv;
static val_t * Unbound;

#define LOG(fmt,...) do { lizplog(fmt,##__VA_ARGS__); }while(0)

static inline void lizplog(char* fmt, ...) {
  va_list va;
  va_start(va, fmt);  
  vprintf(fmt, va);  
  va_end(va);  
}

char * strdump(const char * s) {
	size_t len = strlen(s);
	char * newStr = malloc(len+1);
	strncpy(newStr, s, len);
	newStr[len] = '\0';
	return newStr;
}

val_t * valNew(int t) {
    val_t * v = calloc(1,sizeof(val_t));
	v->type = t;
    return v;
}

void valDel(val_t * v) {
	switch (v->type) {
	case TYPE_ERR:
		free(v->err); break;
	case TYPE_SYM:
		free(v->sym); break;
	case TYPE_STR:
		free(v->str); break;
	case TYPE_LAMBDA:
		free(v->param);
		envDel(v->innerEnv);
		valDel(v->body); break;
	case TYPE_APPLY:
		valDel(v->func);
		valDel(v->formal); break;
	case TYPE_NUM:
	default:
		break;
	}

	free(v);
}

val_t * valErr(const char * fmt, ...) {
    val_t * v = valNew(TYPE_ERR);
    v->err = calloc(512, 1);
    va_list va;
    va_start(va, fmt);
    vsnprintf(v->err, 512, fmt, va);
    va_end(va);
    return v;
}


val_t * valNum(long x) {
	val_t * v = valNew(TYPE_NUM);
	v->num = x;
	return v;
}

val_t * valNumFromStr(const char * s) {
	long n = strtol(s, NULL, 10);
	return errno != ERANGE ? valNum(n) : valErr("Invalid Number!"); 
}

val_t * valStr(const char * s) {
	val_t * v = valNew(TYPE_STR);
	v->str = strdump(s);
	return v;
}

val_t * valSym(const char * s) {
	val_t * v = valNew(TYPE_SYM);
	v->sym = strdump(s);
	return v;
}

val_t * valLambda(const char * s, val_t * b) {
	val_t * v = valNew(TYPE_LAMBDA);
	v->param = strdump(s);
	v->body = b;
	v->innerEnv = envNew();
	/* If in a lambda inner env a peter points to Unbound,
	   it means that the peter has not been bound */
	envInsert(s, Unbound, v->innerEnv); 
	return v;
}

val_t * valApply(val_t * f, val_t * p) {
	val_t * v = valNew(TYPE_APPLY);
	v->func = f;
	v->formal = p;
	return v;
}

val_t * valReadLambda(mpc_ast_t * ast) {
	/* (\<param>.<expr>)*/
	return  valLambda(ast->children[2]->contents, valRead(ast->children[4]));
}

val_t * valReadApply(mpc_ast_t * ast) {
	/* (<expr> <expr>) */
	//printf("Application: childNumber <%d>", ast->children_num);
	return valApply(valRead(ast->children[1]), valRead(ast->children[2]));
}

val_t * valRead(mpc_ast_t * ast) {
    if (!strcmp(ast->tag, ">") && ast->children_num ) {
        return valRead(ast->children[0]);
    } else if (strstr(ast->tag, "number")) {
		return valNumFromStr(ast->contents);
    } else if (strstr(ast->tag, "strings")) {
		return valStr(ast->contents);
	} else if (strstr(ast->tag, "identifier")) {
		return valSym(ast->contents);
    } else if (strstr(ast->tag, "lambda")) {
		return valReadLambda(ast);
	} else if (strstr(ast->tag, "application")) {
		return valReadApply(ast);
	} else {
		return valErr("Invalid AST!");
	}
}

void valPrint(val_t * v) {
	switch (v->type) {
		case TYPE_ERR: printf("<ERR> %s\n", v->err); break;
		case TYPE_STR: printf("<STR> %s\n", v->str); break;
		case TYPE_SYM: printf("<SYM> %s\n", v->sym); break;
		case TYPE_NUM: printf("<NUM> %ld\n", v->num); break;
		case TYPE_LAMBDA: 
					   printf("<LMD> \\%s.", v->param);
					   valPrint(v->body);
					   break;
		case TYPE_APPLY:
					   printf("<APL> ");
					   valPrint(v->func);
					   valPrint(v->formal);
					   break;
		default:
					   printf("ERR!\n");

	}
}

env_t * envNew(void) {
	return calloc(1, sizeof(env_t));
}

void envInsert(const char * s, val_t * v, env_t * e) {
	e->syms = realloc(e->syms, sizeof(char *)*(e->count+1));
	e->vars = realloc(e->vars, sizeof(val_t*)*(e->count+1));
	e->syms[e->count] = s; //Not strdump
	e->vars[e->count] = v;
	e->count++;
}

void envRemove(const char * s, env_t * e) {
	size_t i;
	for (i = 0; i < e->count; ++i) {
		if (!strcmp(e->syms[i], s)) { break; }
	}
	// Here syms are pointers point to v->sym of v of type TYPE_SYM
	// So do NOT free these
	valDel(e->vars[i]);
	memmove(&e->vars[i], &e->vars[i+1], (e->count-i-1)*sizeof(char *));
	memmove(&e->syms[i], &e->syms[i+1], (e->count-i-1)*sizeof(val_t*));
	
	e->count--;
}

void envDel(env_t * e) {
	for (size_t i = 0; i < e->count; ++i) {
		free(e->syms);
		free(e->vars);
	}
}

int envBind(const char * s, val_t * v, env_t * e) {
	for (size_t i = 0; i < e->count; ++i) {
		if (!strcmp(e->syms[i], s) && e->vars[i] == Unbound) {
			e->vars[i] = v;
			return 1;
		}
	}
	return 0;
}

val_t * envFind(const char * s, env_t * e) {
	for (size_t i = 0; i < e->count; ++i) {
		if (!strcmp(e->syms[i], s)) { return e->vars[i]; }
	}

	if (e->parent) {
		return envFind(s, e->parent);
	} else {
		return NULL;
	}
}

val_t * apply(val_t * f, val_t * p, env_t * e) {
	switch (f->type) {
	case TYPE_ERR:
		return f;
	case TYPE_LAMBDA:
		envBind(f->param, p, f->innerEnv);
		return eval(f->body, f->innerEnv);
	case TYPE_SYM:
		return apply(eval(f,e), p, e);
	case TYPE_NUM:
	case TYPE_STR:
	default:
		return valErr("Not a lambda!");
	}
}

val_t * eval(val_t * v, env_t * e) {
	switch (v->type) {
	case TYPE_ERR:
	case TYPE_STR:
	case TYPE_NUM:
	case TYPE_LAMBDA:
		return v;
	case TYPE_SYM:
		return envFind(v->sym,e);
	case TYPE_APPLY:
		return apply(v->func, v->formal, e);
	default:
		return valErr("Eval Error!");
	}
}



int main(void) {
	
    mpc_parser_t * Identifier  = mpc_new("identifier");
    mpc_parser_t * Number      = mpc_new("number");
	mpc_parser_t * Strings     = mpc_new("strings");
	mpc_parser_t * Lambda      = mpc_new("lambda");
	mpc_parser_t * Application = mpc_new("application");
	mpc_parser_t * Expr        = mpc_new("expr");
	mpc_parser_t * Defination  = mpc_new("defination");
	mpc_parser_t * Lizp        = mpc_new("lizp");

    mpca_lang(MPCA_LANG_DEFAULT, 
			"                                                          \
			  number : /-?[0-9]+/ ;									   \
              strings : /\"(\\\\.|[^\"])*\"/ ;                         \
			  defination : \"def\" <identifier> '=' <expr> ;           \
              identifier : /[a-zA-Z_][a-zA-Z0-9_-!@#$^&*<>=|~]*/ ;     \
              lambda : '(' '\\\\' <identifier> '.' <expr> ')' ;        \
              application : '{' <expr> <expr> '}' ;                    \
              expr : <identifier> | <number> | <strings>               \
			       | <application> | <lambda>  ;                       \
			  lizp : /^/ <defination> | <expr> /$/ ;                   \
            ",
			Defination, Number, Strings, Identifier,
            Application, Lambda, Expr, Lizp);

    puts("Lizp Version 0.0.1");
	puts("Init global environment\n");
	GlobalEnv = envNew();
	Unbound = valNew(TYPE_ERR);

    while (1) {
        char * input = readline("lambda > ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lizp, &r)) {
			mpc_ast_print(r.output);
			val_t * v = valRead(r.output);
			valPrint(eval(v,GlobalEnv));
			valDel(v);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

	envDel(GlobalEnv);
	valDel(Unbound);

    mpc_cleanup(5, Lambda, Expr, Identifier, Application, Lizp);

    return 0;
}
