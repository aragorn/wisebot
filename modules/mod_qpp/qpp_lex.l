%{
#include "common_core.h"
#include "mod_qpp2.h"
#include "qpp_yacc.h"
extern int yylineno;
int  tokenpos = 0;
char linebuf[501];
void yyerror(const char* msg);
void save_token(const char* token);

#define SET_OPERAND \
    yylval = new_operand(); \
    if ( yylval == NULL ) { \
        error("error while processing [%s]", yytext); \
        return OPERAND_ERROR; \
    }

#define SET_EXPR \
    yylval->operand_type = OPERAND_EXPR; \
    yylval->result = &yylval->o.expr.result; \
    strncpy( yylval->o.expr.operator_string, yytext, sizeof(yylval->o.expr.operator_string) );

#define TOKEN(name)  { save_token(yytext); return NAME; }

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

\n.*               { 
  strncpy(linebuf, yytext+1, 500); /* Save next line */
  linebuf[500] = '\0';
  tokenpos = 0;
  yyless(1); /* Flush back all but \n to input */
}


SELECT             TOKEN( SELECT )
SEARCH             TOKEN( SEARCH )
WHERE              TOKEN( WHERE )
LIMIT              TOKEN( LIMIT )
BY                 TOKEN( BY )
GROUP              TOKEN( GROUP )
COUNT              TOKEN( COUNT )
GROUP_BY           TOKEN( GROUP_BY )
COUNT_BY           TOKEN( COUNT_BY )
VIRTUAL_ID         TOKEN( VIRTUAL_ID )
ORDER              TOKEN( ORDER )
ORDER_BY           TOKEN( ORDER_BY )
ASC                TOKEN( ASC )
DESC               TOKEN( DESC )
IN                 TOKEN( IN )
IS                 TOKEN( IS )
NULL               TOKEN( NULLX )
BETWEEN            TOKEN( BETWEEN )
AND                TOKEN( AND )
OR                 TOKEN( OR )
NOT                TOKEN( NOT )

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
"."[0-9]*          TOKEN( INTNUM )

  /* 문자열 */
'[^'\n]*'          {
	int c = input();
	unput(c); /* just peeking */
	if (c != '\'') return STRING;
	else yymore();
}

\"[^\"\n]*\"          {
	int c = input();
	unput(c); /* just peeking */
	if (c != '\"') return STRING;
	else yymore();
}

'[^'\n]*$          { yyerror("Unterminated string"); }

[ \t\r]+           ; /* 공백 */
"--".*             ; /* 주석 */

.                  { yyerror("Invalid character"); }

%%

/* */

void yyerror(const char *s)
{
	fprintf(stderr, "line %d, char %d: %s at %s\n%s\n",
            yylineno, tokenpos, s, yytext, linebuf);
    fprintf(stderr, "%*s\n", 1+tokenpos, "^");
}

void save_token(const char *token)
{
	tokenpos += yyleng;
}

