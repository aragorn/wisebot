/* $Id$ */
%{
#include "common_core.h"
#include "mod_qpp2.h"
#include "qpp1_yacc.h"
extern int yylineno;
int  token_pos  = 0;
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

digit     [0-9]
hangul    [\xa1-\xc8][\xa1-\xfe]
alpha     [A-Za-z]
word      [A-Za-z0-9_]

%%

ABSTRACT                  TOKEN( FIELD )
TITLE                     TOKEN( FIELD )
BODY                      TOKEN( FIELD )
FIELD                     TOKEN( FIELD )
TEST                      TOKEN( TEST )

[&-+*/:();]                 TOKEN( yytext[0] )
[^ \t\r\n-+*/:();\"']+     TOKEN( STRING )


'[^'\n]*'  {
   TOKEN( QSTRING );
}

\"[^\"\n]*\"  {
   TOKEN( QSTRING );
}


'[^'\n]*$          { yyerror("Unterminated string"); }

[ \t\r]+           { /* ignore whitespaces */ save_token(yytext); }

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
	//debug("token_pos[%d] += yyleng[%d] with [%s]", token_pos, yyleng, token);
	token_pos += yyleng;
}

