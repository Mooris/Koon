%{
# include "KoonDriver.hh"
# include "parser.hpp"
# include <climits>

static yy::location loc;
%}

%option noyywrap nounput batch debug noinput

id      [a-zA-Z][a-zA-Z_0-9]*
int     [0-9]+
blank   [ \t ]

%{ // Code run each time a pattern is matched.
    # define YY_USER_ACTION  loc.columns (yyleng);
%}

%%

%{ // Code run each time yylex is called.
    loc.step();
%}

{blank}+   loc.step();
[\n]+      loc.lines(yyleng); loc.step ();

"-"         return yy::KoonParser::make_MINUS(loc);
"+"         return yy::KoonParser::make_PLUS(loc);
"*"         return yy::KoonParser::make_STAR(loc);
"/"         return yy::KoonParser::make_SLASH(loc);
"("         return yy::KoonParser::make_LPAREN(loc);
")"         return yy::KoonParser::make_RPAREN(loc);
"="         return yy::KoonParser::make_ASSIGN(loc);
"->"        return yy::KoonParser::make_RARROW(loc);
"."         return yy::KoonParser::make_DOT(loc);
"{"         return yy::KoonParser::make_LBRACE(loc);
"}"         return yy::KoonParser::make_RBRACE(loc);
"decl"      return yy::KoonParser::make_DECL(loc);
"return"    return yy::KoonParser::make_RET(loc);
"class"     return yy::KoonParser::make_CLASS(loc);
"var"       return yy::KoonParser::make_VAR(loc);
"|"         return yy::KoonParser::make_PIPE(loc);

{int}      {
    errno = 0;
    long n = strtol(yytext, NULL, 10);
    if (! (INT_MIN <= n && n <= INT_MAX && errno != ERANGE))
        driver.error(loc, "integer is out of range");
    return yy::KoonParser::make_NUMBER(n, loc);
}

{id}        return yy::KoonParser::make_IDENTIFIER(yytext, loc);
{id}":"     return yy::KoonParser::make_COLID(yytext, loc);

.          driver.error(loc, "invalid character");
<<EOF>>    return yy::KoonParser::make_END(loc);

%%

void KoonDriver::scan_begin()
{
    yy_flex_debug = trace_scanning;
    if (file.empty () || file == "-")
        yyin = stdin;
    else if (!(yyin = fopen (file.c_str (), "r"))) {
        error("cannot open " + file + ": " + strerror(errno));
        exit(EXIT_FAILURE);
    }
}

void KoonDriver::scan_end() {
   fclose (yyin);
}
