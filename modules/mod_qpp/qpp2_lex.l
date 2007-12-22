/* $Id$ */
%{
#include "common_core.h"
#include "mod_qpp2.h"
#include "qpp2_yacc.h"
extern int yylineno;
int  token_pos  = 0;
int  first_line = 1;
#define LINEBUF_LENGTH (500)
char linebuf[LINEBUF_LENGTH+1];
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

%x FULLLINE
%x CLAUSE_SEARCH
%x CLAUSE_SELECT

digit     [0-9]
hangul    [\xa1-\xc8][\xa1-\xfe]
alpha     [A-Za-z]
word      [A-Za-z0-9_]

%%

%{ /* NO RULES BEFORE THIS LINE */
   /* makes sure the first line of the file gets recorded */
   if (first_line == 1)
   {
     /* setup to read the first line of the file */
	 first_line = 0;
	 BEGIN(FULLLINE);
   }
%}

<FULLLINE>\n       {
  /* empty line */
  strncpy(linebuf, yytext, LINEBUF_LENGTH);
  linebuf[LINEBUF_LENGTH] = '\0';
  BEGIN(INITIAL);
  yyless(0);
}

<FULLLINE>[^\n]*\n   {
  /* record the current line, truncating if necessary */
  if (strlen(yytext) > LINEBUF_LENGTH)
  {
    yyerror("Current line is too long.");
	yyerror(yytext);
  }
  strncpy(linebuf, yytext, LINEBUF_LENGTH);
  linebuf[LINEBUF_LENGTH] = '\0';
  debug("linebuf2[%d]=[%s]", yylineno, linebuf);
  token_pos = 0;
  BEGIN(INITIAL);
  yyless(0);
}

"\n"               { BEGIN(FULLLINE); }


SELECT             { BEGIN(CLAUSE_SELECT); TOKEN( SELECT ); };
SEARCH             { BEGIN(CLAUSE_SEARCH); TOKEN( SEARCH ); };
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
TEST               TOKEN( TEST )

<CLAUSE_SEARCH>";"        { BEGIN(INITIAL); TOKEN( yytext[0] ); }
<CLAUSE_SEARCH>"\n"       { BEGIN(FULLLINE); }
<CLAUSE_SEARCH>[-+*/:()]  TOKEN( yytext[0] )
<CLAUSE_SEARCH>[^ \t\r\n-+*/;:()\"']+    TOKEN( STRING )
<CLAUSE_SEARCH>'[^'\n]*'  {
	int c = input();
	unput(c); /* just peeking */
	if (c != '\'') TOKEN( QSTRING )
	else yymore();
}

<CLAUSE_SEARCH>\"[^\"\n]*\"  {
	int c = input();
	unput(c); /* just peeking */
	if (c != '\"') TOKEN( QSTRING )
	else yymore();
}

"="  |
"!=" |
"<>" |
"<"  |
">"  |
"<=" |
">="               TOKEN( COMPARISON )

[`~!@#$%\^&*\(\)_\-+={}|:;"'<>,\./\?]        TOKEN( yytext[0] )

  /* ([^ \t\r`~!@#$%\^&*\(\)_\-+={}|:;"'<>,\./\?]|[^0-9])+  TOKEN( STRING ) */


  /* 문자열 - 필드명,검색어 */
({hangul}|{word})+       TOKEN( STRING )

  /* 숫자 */
{digit}+           |
{digit}+"."{digit}* |
"."{digit}*          TOKEN( INTNUM )

  /* 문자열 */
'[^'\n]*'          {
	int c = input();
	unput(c); /* just peeking */
	if (c != '\'') return QSTRING;
	else yymore();
}

\"[^\"\n]*\"          {
	int c = input();
	unput(c); /* just peeking */
	if (c != '\"') return QSTRING;
	else yymore();
}

'[^'\n]*$          { yyerror("Unterminated string"); }

[ \t\r]+           { /* ignore whitespaces */ save_token(yytext); }
"--".*             { /* ignore comments */ save_token(yytext); }

.                  { yyerror("Invalid character"); }

%%

/* */

void yyerror(const char *s)
{
	fprintf(stderr, "line %d, char %d: %s at %s\n%s\n",
            yylineno, token_pos, s, yytext, linebuf);
    fprintf(stderr, "%*s\n", 1+token_pos, "^");
}

void save_token(const char *token)
{
	debug("token_pos[%d] += yyleng[%d] with [%s]", token_pos, yyleng, token);
	token_pos += yyleng;
}

