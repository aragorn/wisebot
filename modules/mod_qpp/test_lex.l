%{
/* $Id$ */
#include "common_core.h"
#include "mod_qpp2.h"
#include "test_yacc.h"
extern int yylineno;
int  token_pos  = 0;
int  first_line = 1;
void yyerror(const char* msg);
void save_token(const char* token);

#define TOKEN(name)  { save_token(yytext); return name; }

/* About %option
 * 8bit - allow 8bit(hangul) input
 * caseless - case-insensitive match
 * yylineno - enables yylineno variable
 *
 * %array - defines yytext as a char array instead of char pointer.
 *          this enables unput() and other traditional lex functions.
 */
%}

%option 8bit caseless yylineno
%array
%%

SEARCH             TOKEN( SEARCH )
TEST               TOKEN( TEST )

"="  |
"!=" |
"<>" |
"<"  |
">"  |
"<=" |
">="               TOKEN( COMPARISON )
[-+*/:(),.;]       TOKEN( yytext[0] )

  /* 이름 */
[A-Za-z][A-Za-z0-9_]*    TOKEN( NAME )

  /* 숫자 */
[0-9]+             |
[0-9]+"."[0-9]*    |
"."[0-9]*          {
  debug("INTNUM = %d", atoi(yytext));
  yylval = atoi(yytext);
  TOKEN( INTNUM )
}

[ \t\r]+           ; /* 공백 */
"--".*             ; /* 주석 */

.                  { yyerror("Invalid character"); }

%%

/* */

void yyerror(const char *s)
{
	fprintf(stderr, "line %d, char %d: %s at %s\n",
            yylineno, token_pos, s, yytext);
}

void save_token(const char *token)
{
	token_pos += yyleng;
}

