#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>
#include "lizp.h"
#include "mpc.h"

#define ZDEBUG 1
#define LOGBEGIN \
	if(zdebug&&ZDEBUG){ \
		printf("LOG:%-10s:%4d> ", __func__, __LINE__);
#define LOGEND \
		printf("\n"); \
	}

/* Singleton */
static val_t * Unbound;
int zdebug = 0;

static inline void LOG(char* fmt, ...) {
  va_list va;
  va_start(va, fmt);  
  vprintf(fmt, va);  
  va_end(va);  
}

static inline char * strdump(const char * s) {
    size_t len = strlen(s);
    char * newStr = malloc(len+1);
    strncpy(newStr, s, len);
    newStr[len] = '\0';
    return newStr;
}

static inline int isBound(val_t * l) {
    return envFind(l->s, l->innerEnv) != Unbound;
}

val_t * valNew(int t) {
    val_t * v = calloc(1,sizeof(val_t));
    v->type = t;
    return v;
}

void valDel(val_t * v) {
    switch (v->type) {
    case TYPE_ERR:
    case TYPE_SYM:
    case TYPE_STR:
        free(v->s); break;
    case TYPE_LMD:
        free(v->s);
        envDel(v->innerEnv);
        valDel(v->body); break;
    case TYPE_APL:
        valDel(v->left);
        valDel(v->right); break;
    case TYPE_NUM:
    default:
        break;
    }

    free(v);
}

/* Garbage Collect 
 * Maybe need references-count
 * */
void valGC(val_t * v) {
    switch (v->type) {
    case TYPE_ERR:
    case TYPE_STR:
    case TYPE_LMD:
    case TYPE_APL:
    case TYPE_NUM:
		valDel(v);
		break;
	case TYPE_SYM:
    default:
		return;
    }
}

val_t * valErr(const char * fmt, ...) {
    val_t * v = valNew(TYPE_ERR);
    v->s = calloc(512, 1);
    va_list va;
    va_start(va, fmt);
    vsnprintf(v->s, 512, fmt, va);
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
    v->s = strdump(s);
    return v;
}

val_t * valSym(const char * s) {
    val_t * v = valNew(TYPE_SYM);
    v->s = strdump(s);
    return v;
}

val_t * valLambda(const char * s, val_t * b) {
    val_t * v = valNew(TYPE_LMD);
    v->s = strdump(s);
    v->body = b;
    v->innerEnv = envNew();
    /* If in a lambda inner env a pointer points to Unbound,
       it means that the pointer has not been bound */
    envInsert(s, Unbound, v->innerEnv); 
    return v;
}

val_t * valApply(val_t * l, val_t * r) {
    val_t * v = valNew(TYPE_APL);
    v->left = l;
    v->right = r;
    return v;
}

val_t * valReadLambda(mpc_ast_t * ast) {
    /* (\<str>.<expr>)*/
    return valLambda(ast->children[2]->contents, valRead(ast->children[4]));
}

val_t * valReadApply(mpc_ast_t * ast) {
    /* {<expr> <expr>} */
    return valApply(valRead(ast->children[1]), valRead(ast->children[2]));
}

val_t * valRead(mpc_ast_t * ast) {
    if (!strcmp(ast->tag, ">") && ast->children_num ) {
        return valRead(ast->children[0]);
    } else if (strstr(ast->tag, "number")) {
        return valNumFromStr(ast->contents);
    } else if (strstr(ast->tag, "strings")) {
        return valStr(ast->children[1]->contents);
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
        case TYPE_ERR: printf("<ERR %s>", v->s); break;
        case TYPE_STR: printf("\"%s\"", v->s); break;
        case TYPE_NUM: printf("%ld", v->num); break;
        case TYPE_SYM: printf("%s", v->s); break;
        case TYPE_LMD: printf("(\\%s.", v->s);
                       valPrint(v->body);
                       printf(")");
                       break;
        case TYPE_APL: printf("{");
                       valPrint(v->left);
                       printf(" ");
                       valPrint(v->right);
                       printf("}");
                       break;
        default:       valPrint(valErr("UnkownType!"));
    }
}

env_t * envNew(void) {
    return calloc(1, sizeof(env_t));
}

/* Should it free syms and vars ? */
void envDel(env_t * e) {
    for (size_t i = 0; i < e->count; ++i) {
        free(e->syms[i]);
    }
	free(e->syms);
	free(e->vars);
	free(e);
}

int envInsert(const char * s, val_t * v, env_t * e) {
	if (envFind(s,e)) return 0;
    e->syms = realloc(e->syms, sizeof(char *)*(e->count+1));
    e->vars = realloc(e->vars, sizeof(val_t*)*(e->count+1));
    e->syms[e->count] = strdump(s);
    e->vars[e->count] = v;
    e->count++;
    LOGBEGIN
        envPrint(e);
    LOGEND
	return 1;
}

void envRemove(const char * s, env_t * e) {
    size_t i;
    for (i = 0; i < e->count; ++i) {
        if (!strcmp(e->syms[i], s)) { break; }
    }

	free(e->syms[i]);
    valDel(e->vars[i]);
    memmove(&e->vars[i], &e->vars[i+1], (e->count-i-1)*sizeof(char *));
    memmove(&e->syms[i], &e->syms[i+1], (e->count-i-1)*sizeof(val_t*));
    
    e->count--;
}

int envBind(const char * s, val_t * v, env_t * e) {
    for (size_t i = 0; i < e->count; ++i) {
        if (!strcmp(e->syms[i], s) && e->vars[i] == Unbound) {
            e->vars[i] = v;
            LOGBEGIN
                LOG("symbol '%s' is bound to ", s);
                valPrint(v);
				printf(" ");
                envPrint(e);
            LOGEND
            return 1;
        }
    }
    return 0;
}

int envDeBind(const char * s, env_t * e) {
    for (size_t i = 0; i < e->count; ++i) {
        if (!strcmp(e->syms[i], s) && e->vars[i] != Unbound) {
            e->vars[i] = Unbound;
            LOGBEGIN
                LOG("symbol '%s' is bound to ", s);
                valPrint(Unbound);
                envPrint(e);
            LOGEND
            return 1;
        }
    }
    return 0;
}

val_t * envFind(const char * s, env_t * e) {
    for (size_t i = 0; i < e->count; ++i) {
        if (!strcmp(e->syms[i], s)) {
            LOGBEGIN
                envPrint(e);
            LOGEND
            return e->vars[i];
        }
    }

    if (e->parent) {
        return envFind(s, e->parent);
    } 
    LOGBEGIN
        printf("symbol '%s' not found!", s);
    LOGEND
    return NULL;
}

void envPrint(env_t * e) {
    printf("Env[%lu]={ ", e->count);
    for (size_t i = 0; i < e->count; ++i) {
        printf("[%s,", e->syms[i]);
        valPrint(e->vars[i]);
        printf("] ");
    }
    printf("}");
}

val_t * apply(val_t * l, val_t * r, env_t * e) {
    switch (l->type) {
    case TYPE_ERR:
        return l;
    case TYPE_LMD:
		envDeBind(l->s, l->innerEnv);
        envBind(l->s, r, l->innerEnv);
		return eval(l->body, l->innerEnv);
    case TYPE_APL:
    case TYPE_SYM:
        return apply(eval(l,e), r, e);
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
        return v;
    case TYPE_LMD: //Maybe return a closure?
        return v;
    case TYPE_SYM:
        return envFind(v->s,e)?:valErr("Symbol '%s' is not bound!", v->s);
    case TYPE_APL:
        return apply(eval(v->left,e), eval(v->right,e), e);
    default:
        return valErr("Eval Error!");
    }
}

void linkScope(val_t * l, env_t * e) {
    /* {{(\x.(\y.{x y})) (\z.z)} "2"} */
    if (l->type == TYPE_LMD) {
        l->innerEnv->parent = e; 
        linkScope(l->body, l->innerEnv);
		LOGBEGIN
			envPrint(l->innerEnv);
			envPrint(e);
		LOGEND
        return;
    } else if (l->type == TYPE_APL) {
        linkScope(l->left, e);
        linkScope(l->right, e);
        return;
	} else if (l->type == TYPE_SYM) {
		linkScope(envFind(l->s,e), e);
		return;
	}
}

void def(mpc_ast_t * ast, env_t * e) {
    /* def <smb> = <expr> */
	char * s = ast->children[1]->contents;
    int r = envInsert(s, eval(valRead(ast->children[3]),e), e);
	if (!r) printf("Error: '%s' has already been bound\n", s);
}

void interprete(mpc_ast_t * ast, env_t * e) {
    mpc_ast_t * definiation = ast->children[1];
    if (strstr(definiation->tag, "definition") ) {
        def(definiation, e);
    } else {
        val_t * v = valRead(ast);
        LOGBEGIN valPrint(v); LOGEND

        linkScope(v, e);

        val_t * ve = (eval(v, e));

        LOGBEGIN printf("Eval finish"); LOGEND

        valPrint(ve);
        printf("\n");
    }
}

int main(int argc, char ** argv) {

    if (argc >= 2 && !strcmp(argv[1], "-v")) {
        zdebug = 1;
    }
    
    mpc_parser_t * Identifier  = mpc_new("identifier");
    mpc_parser_t * Number      = mpc_new("number");
    mpc_parser_t * Strings     = mpc_new("strings");
    mpc_parser_t * Lambda      = mpc_new("lambda");
    mpc_parser_t * Application = mpc_new("application");
    mpc_parser_t * Expr        = mpc_new("expr");
    mpc_parser_t * Definition  = mpc_new("definition");
    mpc_parser_t * Lizp        = mpc_new("lizp");

    mpca_lang(MPCA_LANG_DEFAULT, 
            "                                                          \
              number : /-?[0-9]+/ ;                                    \
              strings : '\"' /(\\\\.|[^\"])*/ '\"';                    \
              definition : \"def\" <identifier> '=' <expr> ;           \
              identifier : /[a-zA-Z_][a-zA-Z0-9_-!@#$^&*<>=|~]*/ ;     \
              lambda : '(' '\\\\' <identifier> '.' <expr> ')' ;        \
              application : '{' <expr> <expr> '}' ;                    \
              expr : <identifier> | <number> | <strings>               \
                   | <application> | <lambda>  ;                       \
              lizp : /^/ <definition> | <expr> /$/ ;                   \
            ",
            Definition, Number, Strings, Identifier,
            Application, Lambda, Expr, Lizp);

    puts("Lizp Version 0.0.1");
    puts("Init global environment\n");

    Unbound = valNew(TYPE_ERR);
    Unbound->s = "(Singleton)";
    val_t *GlobalEnvEntry = valLambda("(GLOBAL)", NULL);
    GlobalEnvEntry->body = valStr("(GLOBAL)");
    GlobalEnvEntry->innerEnv->parent = NULL;
    env_t * GlobalEnv = GlobalEnvEntry->innerEnv;

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
    while (1) {
        char * input = readline("lizp> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Lizp, &r)) {
            LOGBEGIN mpc_ast_print(r.output); LOGEND
            interprete(r.output, GlobalEnv);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }

#pragma clang diagnostic pop

    valDel(GlobalEnvEntry);
    valDel(Unbound);

    mpc_cleanup(5, Lambda, Expr, Identifier, Application, Lizp);

    return 0;
}
