#include "lizp.h"
#include "mpc.h"
#include <editline/readline.h>
#include <stdio.h>
#include <stdlib.h>

static env_t * globalEnv;
static val_t * unboundVal;

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

val_t * valNew(int type) {
    val_t * v = calloc(1,sizeof(val_t));
	v->type = type;
    return v;
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

val_t * valLambda(const char * param, val_t * body) {
	val_t * v = valNew(TYPE_LAMBDA);
	v->param = strdump(param);
	v->body = body;
	v->innerEnv = envNew();
	/* If in a lambda inner env a parameter points to Unbound,
	   it means that the parameter has not been bound */
	envAdd(param, unboundVal, v->innerEnv); 
	return v;
}

val_t * valReadLambda(mpc_ast_t * ast) {
	/* (\<param>.<expr>)*/
	return  valLambda(ast->children[2]->contents, valRead(ast->children[4]));
}

val_t * valReadApply(mpc_ast_t * ast) {
	/* (<expr> <expr>) */
	//printf("Application: childNumber <%d>", ast->children_num);
	return eval(valRead(ast->children[1]),valRead(ast->children[2]), globalEnv);
}

val_t * eval(val_t * lambda, val_t * param, env_t * env) {
	if (param == NULL) {
		return envFind(lambda->sym, env);
	}
	if (lambda->type == TYPE_SYM) {
		return eval(envFind(lambda->sym, env), param, env);
	}
	if (param->type == TYPE_SYM) {
		return eval(lambda, envFind(param->sym, env), env);
	}
	if (lambda->type == TYPE_LAMBDA) {
		if (envBind(lambda->body->sym, param, lambda->innerEnv))
			return eval(lambda->body, NULL, lambda->innerEnv);
	}
	return valErr("Eval failed!");
}

int envBind(const char * s, val_t * v, env_t * e) {
	for (size_t i = 0; i < e->count; ++i) {
		if (!strcmp(e->syms[i], s) && e->vars[i] == unboundVal) {
			e->vars[i] = v;
			return 1;
		}
	}
	return 0;
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
					   break;
		default:
					   printf("ERR!\n");

	}
}

env_t * envNew(void) {
	return calloc(1, sizeof(env_t));
}

void envAdd(const char * s, val_t * v, env_t * e) {
	e->syms = realloc(e->syms, sizeof(char *)*(e->count+1));
	e->vars = realloc(e->vars, sizeof(val_t*)*(e->count+1));
	e->syms[e->count] = s;
	e->vars[e->count] = v;
	e->count++;
}

void envDel(const char * s, env_t * e) {
	for (size_t i = 0; i < e->count; i++) {
		if (!strcmp(e->syms[i], s))
			break;
	}
}

val_t * envFind(const char * s, env_t * e) {
	for (size_t i = 0; i < e->count; ++i) {
		if (!strcmp(e->syms[i], s)) {
			return e->vars[i];
		}
	}

	if (e->parent) {
		return envFind(s, e->parent);
	} else {
		return NULL;
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
              application : '(' <expr> <expr> ')' ;                    \
              expr : <identifier> | <number> | <strings>               \
			       | <application> | <lambda>  ;                       \
			  lizp : /^/ <defination> | <expr> /$/ ;                   \
            ",
			Defination, Number, Strings, Identifier,
            Application, Lambda, Expr, Lizp);

    puts("Lizp Version 0.0.1");
	puts("Init global environment\n");
	globalEnv = envNew();
	unboundVal = valNew(TYPE_ERR);

    while (1) {
        char * input = readline("lambda > ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lizp, &r)) {
			mpc_ast_print(r.output);
			valPrint(valRead(r.output));
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

    mpc_cleanup(5, Lambda, Expr, Identifier, Application, Lizp);

    return 0;
}
