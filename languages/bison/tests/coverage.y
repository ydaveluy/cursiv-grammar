%language "C++"
%define api.pure full
%defines "parser.h"
%error-verbose
%expect 0
%expect-rr 0
%glr-parser
%initial-action { init(); }
%name-prefix "cov_"
%output "cov.tab.c"
%param { int x }
%pure-parser
%parse-param { int* n }
%lex-param { void* scanner }
%require "3.0\n"
%skeleton "lalr1.cc"
%token-table
%verbose
%yacc
%file-prefix "cov"
%locations
%debug
%no-lines
%nondeterministic-parser
;

%{
#include <cstdio>
%}

%code top { #include <cstdio> }
%code requires { struct Ctx; }
%union { int i; char* s; }

%default-prec
%no-default-prec

%nterm <i> start
%token <i> NUM 42 "number"
%token FOO BAR(BASE) "fooalias"
%token PLUS "+" MINUS
%left <i> PLUS2 MINUS2 <s> CONCAT
%right '^'
%nonassoc '<'
%binary '*'
%precedence UMINUS
%type <i> expr start
%destructor { do_free($$); } expr <*>
%printer { fprintf(yyo, "%d", $$); } NUM <>

%start start

%%

start
    : expr { $$ = $1; }
    ;

line
    : '\n' { }
    ;

expr
    : NUM
    | expr PLUS expr %prec UMINUS %dprec 1 %merge <i>
    | expr[a] MINUS expr[b] { $$ = $a - $b; }
    | %empty
    | %?{ cond() } expr
    ;

%type <i> extra;

%%
int cov_error(const char* s) { return 0; }
